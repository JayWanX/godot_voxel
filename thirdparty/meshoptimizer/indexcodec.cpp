// This file is part of meshoptimizer library; see meshoptimizer.h for version/license details
#include "meshoptimizer.h"

#include <assert.h>
#include <string.h>

MESHOPTIMIZER_VOXEL_NAMESPACE_BEGIN

// 此作品基于：
// Fabian Giesen. 简单的无损索引缓冲区压缩及后续. 2013
// Conor Stokes. 顶点缓存优化的索引缓冲区压缩. 2014
namespace meshopt
{

const unsigned char kIndexHeader = 0xe0;
const unsigned char kSequenceHeader = 0xd0;

static int gEncodeIndexVersion = 1;
const int kDecodeIndexVersion = 1;

typedef unsigned int VertexFifo[16];
typedef unsigned int EdgeFifo[16][2];

static const unsigned int kTriangleIndexOrder[3][3] = {
    {0, 1, 2},
    {1, 2, 0},
    {2, 0, 1},
};

static const unsigned char kCodeAuxEncodingTable[16] = {
    0x00, 0x76, 0x87, 0x56, 0x67, 0x78, 0xa9, 0x86, 0x65, 0x89, 0x68, 0x98, 0x01, 0x69,
    0, 0, // 最后两个条目不用于编码
};

static int rotateTriangle(unsigned int a, unsigned int b, unsigned int c, unsigned int next)
{
	(void)a;

	return (b == next) ? 1 : (c == next ? 2 : 0);
}

static int getEdgeFifo(EdgeFifo fifo, unsigned int a, unsigned int b, unsigned int c, size_t offset)
{
	for (int i = 0; i < 16; ++i)
	{
		size_t index = (offset - 1 - i) & 15;

		unsigned int e0 = fifo[index][0];
		unsigned int e1 = fifo[index][1];

		if (e0 == a && e1 == b)
			return (i << 2) | 0;
		if (e0 == b && e1 == c)
			return (i << 2) | 1;
		if (e0 == c && e1 == a)
			return (i << 2) | 2;
	}

	return -1;
}

static void pushEdgeFifo(EdgeFifo fifo, unsigned int a, unsigned int b, size_t& offset)
{
	fifo[offset][0] = a;
	fifo[offset][1] = b;
	offset = (offset + 1) & 15;
}

static int getVertexFifo(VertexFifo fifo, unsigned int v, size_t offset)
{
	for (int i = 0; i < 16; ++i)
	{
		size_t index = (offset - 1 - i) & 15;

		if (fifo[index] == v)
			return i;
	}

	return -1;
}

static void pushVertexFifo(VertexFifo fifo, unsigned int v, size_t& offset, int cond = 1)
{
	fifo[offset] = v;
	offset = (offset + cond) & 15;
}

static void encodeVByte(unsigned char*& data, unsigned int v)
{
	// 将 32 位值编码为最多 5 个 7 位组
	do
	{
		*data++ = (v & 127) | (v > 127 ? 128 : 0);
		v >>= 7;
	} while (v);
}

static unsigned int decodeVByte(const unsigned char*& data)
{
	unsigned char lead = *data++;

	// 快速路径：单字节
	if (lead < 128)
		return lead;

	// 慢速路径：最多 4 个额外字节
	// 注意：该循环总会终止，这对畸形数据很重要
	unsigned int result = lead & 127;
	unsigned int shift = 7;

	for (int i = 0; i < 4; ++i)
	{
		unsigned char group = *data++;
		result |= unsigned(group & 127) << shift;
		shift += 7;

		if (group < 128)
			break;
	}

	return result;
}

static void encodeIndex(unsigned char*& data, unsigned int index, unsigned int last)
{
	unsigned int d = index - last;
	unsigned int v = (d << 1) ^ (int(d) >> 31);

	encodeVByte(data, v);
}

static unsigned int decodeIndex(const unsigned char*& data, unsigned int last)
{
	unsigned int v = decodeVByte(data);
	unsigned int d = (v >> 1) ^ -int(v & 1);

	return last + d;
}

static int getCodeAuxIndex(unsigned char v, const unsigned char* table)
{
	for (int i = 0; i < 16; ++i)
		if (table[i] == v)
			return i;

	return -1;
}

static void writeTriangle(void* destination, size_t offset, size_t index_size, unsigned int a, unsigned int b, unsigned int c)
{
	if (index_size == 2)
	{
		static_cast<unsigned short*>(destination)[offset + 0] = (unsigned short)(a);
		static_cast<unsigned short*>(destination)[offset + 1] = (unsigned short)(b);
		static_cast<unsigned short*>(destination)[offset + 2] = (unsigned short)(c);
	}
	else
	{
		static_cast<unsigned int*>(destination)[offset + 0] = a;
		static_cast<unsigned int*>(destination)[offset + 1] = b;
		static_cast<unsigned int*>(destination)[offset + 2] = c;
	}
}

} // namespace meshopt

size_t meshopt_encodeIndexBuffer(unsigned char* buffer, size_t buffer_size, const unsigned int* indices, size_t index_count)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);

	// 最小有效编码为：header、每个三角形 1 字节，以及一个 16 字节的 codeaux 表
	if (buffer_size < 1 + index_count / 3 + 16)
		return 0;

	int version = gEncodeIndexVersion;

	buffer[0] = (unsigned char)(kIndexHeader | version);

	EdgeFifo edgefifo;
	memset(edgefifo, -1, sizeof(edgefifo));

	VertexFifo vertexfifo;
	memset(vertexfifo, -1, sizeof(vertexfifo));

	size_t edgefifooffset = 0;
	size_t vertexfifooffset = 0;

	unsigned int next = 0;
	unsigned int last = 0;

	unsigned char* code = buffer + 1;
	unsigned char* data = code + index_count / 3;
	unsigned char* data_safe_end = buffer + buffer_size - 16;

	int fecmax = version >= 1 ? 13 : 15;

	// use static encoding table; it's possible to pack the result and then build an optimal table and repack
	// 目前我们保持简单，使用根据训练网格集上的符号频率生成的表
	const unsigned char* codeaux_table = kCodeAuxEncodingTable;

	for (size_t i = 0; i < index_count; i += 3)
	{
		// 确保有足够空间写入一个三角形
		// 每个三角形最多写入 16 字节：1b 用于 codeaux，每个空闲索引 5b
		// 在此之后，我们可以确保无需额外的边界检查即可写入
		if (data > data_safe_end)
			return 0;

		int fer = getEdgeFifo(edgefifo, indices[i + 0], indices[i + 1], indices[i + 2], edgefifooffset);

		if (fer >= 0 && (fer >> 2) < 15)
		{
			// 注意：getEdgeFifo 通过将 a/b 与现有边匹配来隐式旋转三角形
			const unsigned int* order = kTriangleIndexOrder[fer & 3];

			unsigned int a = indices[i + order[0]], b = indices[i + order[1]], c = indices[i + order[2]];

			// 编码边索引和顶点 fifo 索引（next 或 free 索引）
			int fe = fer >> 2;
			int fc = getVertexFifo(vertexfifo, c, vertexfifooffset);

			int fec = (fc >= 1 && fc < fecmax) ? fc : (c == next ? (next++, 0) : 15);

			if (fec == 15 && version >= 1)
			{
				// 编码 last-1 和 last+1，以优化类似条带的序列
				if (c + 1 == last)
					fec = 13, last = c;
				if (c == last + 1)
					fec = 14, last = c;
			}

			*code++ = (unsigned char)((fe << 4) | fec);

			// 注意，我们需要更新最后一个索引，因为空闲索引是经 delta 编码的
			if (fec == 15)
				encodeIndex(data, c, last), last = c;

			// 只需推入第三个顶点，因为前两个很可能已在顶点 fifo 中
			if (fec == 0 || fec >= fecmax)
				pushVertexFifo(vertexfifo, c, vertexfifooffset);

			// 只需向边 fifo 推入两条新边，因为第三条已经在其中
			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
		else
		{
			int rotation = rotateTriangle(indices[i + 0], indices[i + 1], indices[i + 2], next);
			const unsigned int* order = kTriangleIndexOrder[rotation];

			unsigned int a = indices[i + order[0]], b = indices[i + order[1]], c = indices[i + order[2]];

			// if a/b/c are 0/1/2, we emit a reset code
			bool reset = false;

			if (a == 0 && b == 1 && c == 2 && next > 0 && version >= 1)
			{
				reset = true;
				next = 0;

				// 重置顶点 fifo，确保将来不会意外引用其中的顶点
				// 这确保 next 能继续递增而不会卡住
				memset(vertexfifo, -1, sizeof(vertexfifo));
			}

			int fb = getVertexFifo(vertexfifo, b, vertexfifooffset);
			int fc = getVertexFifo(vertexfifo, c, vertexfifooffset);

			// 旋转后，a 几乎总是等于 next，因此不会在 a 的 FIFO 编码上浪费位数
			// note: decoder implicitly assumes that if feb=fec=0, then fea=0 (reset code); this is enforced by rotation
			int fea = (a == next) ? (next++, 0) : 15;
			int feb = (fb >= 0 && fb < 14) ? fb + 1 : (b == next ? (next++, 0) : 15);
			int fec = (fc >= 0 && fc < 14) ? fc + 1 : (c == next ? (next++, 0) : 15);

			// 我们尽可能用 4 位加表编码 feb 和 fec，否则用完整字节
			unsigned char codeaux = (unsigned char)((feb << 4) | fec);
			int codeauxindex = getCodeAuxIndex(codeaux, codeaux_table);

			// <14 编码为 codeaux 表的索引，14 编码 fea=0，15 编码 fea=15
			if (fea == 0 && codeauxindex >= 0 && codeauxindex < 14 && !reset)
			{
				*code++ = (unsigned char)((15 << 4) | codeauxindex);
			}
			else
			{
				*code++ = (unsigned char)((15 << 4) | 14 | fea);
				*data++ = codeaux;
			}

			// 注意，我们需要更新最后一个索引，因为空闲索引是经 delta 编码的
			if (fea == 15)
				encodeIndex(data, a, last), last = a;

			if (feb == 15)
				encodeIndex(data, b, last), last = b;

			if (fec == 15)
				encodeIndex(data, c, last), last = c;

			// 只推入尚未在 fifo 中的顶点
			if (fea == 0 || fea == 15)
				pushVertexFifo(vertexfifo, a, vertexfifooffset);

			if (feb == 0 || feb == 15)
				pushVertexFifo(vertexfifo, b, vertexfifooffset);

			if (fec == 0 || fec == 15)
				pushVertexFifo(vertexfifo, c, vertexfifooffset);

			// all three edges aren't in the fifo; pushing all of them is important so that we can match them for later triangles
			pushEdgeFifo(edgefifo, b, a, edgefifooffset);
			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);
		}
	}

	// 确保有足够空间写入 codeaux 表
	if (data > data_safe_end)
		return 0;

	// add codeaux encoding table to the end of the stream; this is used for decoding codeaux *and* as padding
	// 解码时需要填充，以便能假定每个三角形最多编码为 16 字节的额外数据
	// 这足以为辅助字节加每个 varint 索引 5 字节留出空间，这是任何输入的最坏情况
	for (size_t i = 0; i < 16; ++i)
	{
		// 解码器假定表条目永远不会引用单独编码的索引
		assert((codeaux_table[i] & 0xf) != 0xf && (codeaux_table[i] >> 4) != 0xf);

		*data++ = codeaux_table[i];
	}

	// 由于我们将重启动编码为不带表引用的 codeaux，需要确保 00 被编码为表引用
	assert(codeaux_table[0] == 0);

	assert(data >= buffer + index_count / 3 + 16);
	assert(data <= buffer + buffer_size);

	return data - buffer;
}

size_t meshopt_encodeIndexBufferBound(size_t index_count, size_t vertex_count)
{
	assert(index_count % 3 == 0);

	// 计算每个索引所需的位数
	unsigned int vertex_bits = 1;

	while (vertex_bits < 32 && vertex_count > size_t(1) << vertex_bits)
		vertex_bits++;

	// 最坏情况编码为 2 字节头 + 3 个 varint-7 编码的索引增量
	unsigned int vertex_groups = (vertex_bits + 1 + 6) / 7;

	return 1 + (index_count / 3) * (2 + 3 * vertex_groups) + 16;
}

void meshopt_encodeIndexVersion(int version)
{
	assert(unsigned(version) <= unsigned(meshopt::kDecodeIndexVersion));

	meshopt::gEncodeIndexVersion = version;
}

int meshopt_decodeIndexVersion(const unsigned char* buffer, size_t buffer_size)
{
	if (buffer_size < 1)
		return -1;

	unsigned char header = buffer[0];

	if ((header & 0xf0) != meshopt::kIndexHeader && (header & 0xf0) != meshopt::kSequenceHeader)
		return -1;

	int version = header & 0x0f;
	if (version > meshopt::kDecodeIndexVersion)
		return -1;

	return version;
}

int meshopt_decodeIndexBuffer(void* destination, size_t index_count, size_t index_size, const unsigned char* buffer, size_t buffer_size)
{
	using namespace meshopt;

	assert(index_count % 3 == 0);
	assert(index_size == 2 || index_size == 4);

	// 最小有效编码为：header、每个三角形 1 字节，以及一个 16 字节的 codeaux 表
	if (buffer_size < 1 + index_count / 3 + 16)
		return -2;

	if ((buffer[0] & 0xf0) != kIndexHeader)
		return -1;

	int version = buffer[0] & 0x0f;
	if (version > kDecodeIndexVersion)
		return -1;

	EdgeFifo edgefifo;
	memset(edgefifo, -1, sizeof(edgefifo));

	VertexFifo vertexfifo;
	memset(vertexfifo, -1, sizeof(vertexfifo));

	size_t edgefifooffset = 0;
	size_t vertexfifooffset = 0;

	unsigned int next = 0;
	unsigned int last = 0;

	int fecmax = version >= 1 ? 13 : 15;

	// 由于我们在尾部存储 16 字节的 codeaux 表，三角形数据必须在 data_safe_end 之前开始
	const unsigned char* code = buffer + 1;
	const unsigned char* data = code + index_count / 3;
	const unsigned char* data_safe_end = buffer + buffer_size - 16;

	const unsigned char* codeaux_table = data_safe_end;

	for (size_t i = 0; i < index_count; i += 3)
	{
		// 确保有足够数据读取一个三角形
		// 每个三角形最多读取 16 字节数据：1b 用于 codeaux，每个空闲索引 5b
		// 在此之后，我们可以确保无需额外的边界检查即可读取
		if (data > data_safe_end)
			return -2;

		unsigned char codetri = *code++;

		if (codetri < 0xf0)
		{
			int fe = codetri >> 4;

			// fifo 读取在 16 条目缓冲区上做回绕
			unsigned int a = edgefifo[(edgefifooffset - 1 - fe) & 15][0];
			unsigned int b = edgefifo[(edgefifooffset - 1 - fe) & 15][1];
			unsigned int c = 0;

			int fec = codetri & 15;

			// 注意：这是整个解码器中最常见的路径
			// 在此 if 内我们尽量保持无分支（通过使用 cmov 等），因为这几处分支不可预测
			if (fec < fecmax)
			{
				// fifo 读取在 16 条目缓冲区上做回绕
				unsigned int cf = vertexfifo[(vertexfifooffset - 1 - fec) & 15];
				c = (fec == 0) ? next : cf;

				int fec0 = fec == 0;
				next += fec0;

				// 推送顶点 fifo 必须与编码步骤*完全*一致，否则数据将无法正确解码
				pushVertexFifo(vertexfifo, c, vertexfifooffset, fec0);
			}
			else
			{
				// fec - (fec ^ 3) 将 13、14 解码为 -1、1
				// 注意，我们需要更新最后一个索引，因为空闲索引是经 delta 编码的
				last = c = (fec != 15) ? last + (fec - (fec ^ 3)) : decodeIndex(data, last);

				// push 顶点/边 fifo 必须与编码步骤完全一致，否则数据将无法正确解码
				pushVertexFifo(vertexfifo, c, vertexfifooffset);
			}

			// 推送边 fifo 必须与编码步骤*完全*一致，否则数据将无法正确解码
			pushEdgeFifo(edgefifo, c, b, edgefifooffset);
			pushEdgeFifo(edgefifo, a, c, edgefifooffset);

			// 输出三角形
			writeTriangle(destination, i, index_size, a, b, c);
		}
		else
		{
			// 快速路径：从表读取 codeaux
			if (codetri < 0xfe)
			{
				unsigned char codeaux = codeaux_table[codetri & 15];

				// 注意：表不能包含 feb/fec=15
				int feb = codeaux >> 4;
				int fec = codeaux & 15;

				// fifo 读取在 16 条目缓冲区上做回绕
				// 还要注意，在解码索引之前我们会为所有三个顶点递增 next——这与编码器行为一致
				unsigned int a = next++;

				unsigned int bf = vertexfifo[(vertexfifooffset - feb) & 15];
				unsigned int b = (feb == 0) ? next : bf;

				int feb0 = feb == 0;
				next += feb0;

				unsigned int cf = vertexfifo[(vertexfifooffset - fec) & 15];
				unsigned int c = (fec == 0) ? next : cf;

				int fec0 = fec == 0;
				next += fec0;

				// 输出三角形
				writeTriangle(destination, i, index_size, a, b, c);

				// push 顶点/边 fifo 必须与编码步骤完全一致，否则数据将无法正确解码
				pushVertexFifo(vertexfifo, a, vertexfifooffset);
				pushVertexFifo(vertexfifo, b, vertexfifooffset, feb0);
				pushVertexFifo(vertexfifo, c, vertexfifooffset, fec0);

				pushEdgeFifo(edgefifo, b, a, edgefifooffset);
				pushEdgeFifo(edgefifo, c, b, edgefifooffset);
				pushEdgeFifo(edgefifo, a, c, edgefifooffset);
			}
			else
			{
				// 慢速路径：读取完整字节作为 codeaux，而非使用表查找
				unsigned char codeaux = *data++;

				int fea = codetri == 0xfe ? 0 : 15;
				int feb = codeaux >> 4;
				int fec = codeaux & 15;

				// 重置：codeaux 为 0，但编码为非表形式
				if (codeaux == 0)
					next = 0;

				// fifo 读取在 16 条目缓冲区上做回绕
				// 还要注意，在解码索引之前我们会为所有三个顶点递增 next——这与编码器行为一致
				unsigned int a = (fea == 0) ? next++ : 0;
				unsigned int b = (feb == 0) ? next++ : vertexfifo[(vertexfifooffset - feb) & 15];
				unsigned int c = (fec == 0) ? next++ : vertexfifo[(vertexfifooffset - fec) & 15];

				// 注意，我们需要更新最后一个索引，因为空闲索引是经 delta 编码的
				if (fea == 15)
					last = a = decodeIndex(data, last);

				if (feb == 15)
					last = b = decodeIndex(data, last);

				if (fec == 15)
					last = c = decodeIndex(data, last);

				// 输出三角形
				writeTriangle(destination, i, index_size, a, b, c);

				// push 顶点/边 fifo 必须与编码步骤完全一致，否则数据将无法正确解码
				pushVertexFifo(vertexfifo, a, vertexfifooffset);
				pushVertexFifo(vertexfifo, b, vertexfifooffset, (feb == 0) | (feb == 15));
				pushVertexFifo(vertexfifo, c, vertexfifooffset, (fec == 0) | (fec == 15));

				pushEdgeFifo(edgefifo, b, a, edgefifooffset);
				pushEdgeFifo(edgefifo, c, b, edgefifooffset);
				pushEdgeFifo(edgefifo, a, c, edgefifooffset);
			}
		}
	}

	// 我们应该已读取所有数据字节，并在数据与 codeaux 表之间的边界处停止
	if (data != data_safe_end)
		return -3;

	return 0;
}

size_t meshopt_encodeIndexSequence(unsigned char* buffer, size_t buffer_size, const unsigned int* indices, size_t index_count)
{
	using namespace meshopt;

	// 最小有效编码为：header、每个索引 1 字节，以及一个 4 字节的尾部
	if (buffer_size < 1 + index_count + 4)
		return 0;

	int version = gEncodeIndexVersion;

	buffer[0] = (unsigned char)(kSequenceHeader | version);

	unsigned int last[2] = {};
	unsigned int current = 0;

	unsigned char* data = buffer + 1;
	unsigned char* data_safe_end = buffer + buffer_size - 4;

	for (size_t i = 0; i < index_count; ++i)
	{
		// 确保有足够数据可供写入
		// each index writes at most 5 bytes of data; there's a 4 byte tail after data_safe_end
		// 在此之后，我们可以确保无需额外的边界检查即可写入
		if (data >= data_safe_end)
			return 0;

		unsigned int index = indices[i];

		// 这是一种启发式方法，当增量过大时在基线之间切换
		// 我们希望编码增量能放入一个字节（7 位），但 2 位被用于符号和基线索引
		// 目前当增量过大时我们立即切换基线 - 这可以任意调整
		int cd = int(index - last[current]);
		current ^= ((cd < 0 ? -cd : cd) >= 30);

		// 编码相对最后索引的增量
		unsigned int d = index - last[current];
		unsigned int v = (d << 1) ^ (int(d) >> 31);

		// 注意：低位编码将用于重建的最后基线的索引
		encodeVByte(data, (v << 1) | current);

		// 为使用它的下一次迭代更新 last
		last[current] = index;
	}

	// 确保有足够空间写入尾部
	if (data > data_safe_end)
		return 0;

	for (int k = 0; k < 4; ++k)
		*data++ = 0;

	return data - buffer;
}

size_t meshopt_encodeIndexSequenceBound(size_t index_count, size_t vertex_count)
{
	// 计算每个索引所需的位数
	unsigned int vertex_bits = 1;

	while (vertex_bits < 32 && vertex_count > size_t(1) << vertex_bits)
		vertex_bits++;

	// 最坏情况编码为 K 位值用 1 个 varint-7 编码的索引增量外加 1 位
	unsigned int vertex_groups = (vertex_bits + 1 + 1 + 6) / 7;

	return 1 + index_count * vertex_groups + 4;
}

int meshopt_decodeIndexSequence(void* destination, size_t index_count, size_t index_size, const unsigned char* buffer, size_t buffer_size)
{
	using namespace meshopt;

	// 最小有效编码为：header、每个索引 1 字节，以及一个 4 字节的尾部
	if (buffer_size < 1 + index_count + 4)
		return -2;

	if ((buffer[0] & 0xf0) != kSequenceHeader)
		return -1;

	int version = buffer[0] & 0x0f;
	if (version > kDecodeIndexVersion)
		return -1;

	const unsigned char* data = buffer + 1;
	const unsigned char* data_safe_end = buffer + buffer_size - 4;

	unsigned int last[2] = {};

	for (size_t i = 0; i < index_count; ++i)
	{
		// 确保有足够数据可供读取
		// each index reads at most 5 bytes of data; there's a 4 byte tail after data_safe_end
		// 在此之后，我们可以确保无需额外的边界检查即可读取
		if (data >= data_safe_end)
			return -2;

		unsigned int v = decodeVByte(data);

		// 解码最后基线的索引
		unsigned int current = v & 1;
		v >>= 1;

		// 将索引重建为增量
		unsigned int d = (v >> 1) ^ -int(v & 1);
		unsigned int index = last[current] + d;

		// 为使用它的下一次迭代更新 last
		last[current] = index;

		if (index_size == 2)
		{
			static_cast<unsigned short*>(destination)[i] = (unsigned short)(index);
		}
		else
		{
			static_cast<unsigned int*>(destination)[i] = index;
		}
	}

	// 我们应该已读取所有数据字节，并在数据与尾部之间的边界处停止
	if (data != data_safe_end)
		return -3;

	return 0;
}

MESHOPTIMIZER_VOXEL_NAMESPACE_END