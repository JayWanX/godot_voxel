#include "../../generators/graph/node_type_db.h"
#include "../../util/containers/std_vector.h"
#include "../../util/errors.h"
#include "../../util/godot/classes/file_access.h"
#include "../../util/godot/classes/xml_parser.h"
#include "../../util/godot/core/array.h"
#include <unordered_map>

namespace voxel {

// 临时数据，用于替代直接修改 NodeTypeDB
struct GraphNodeDocumentation {
	String name;
	String description;
	String category;
};

GraphNodeDocumentation *find_node_by_name(StdVector<GraphNodeDocumentation> &nodes, String name) {
	for (GraphNodeDocumentation &node : nodes) {
		if (node.name == name) {
			return &node;
		}
	}
	return nullptr;
}

bool parse_graph_nodes_doc_xml(XMLParser &parser, StdVector<GraphNodeDocumentation> &nodes) {
	VOXEL_ASSERT_RETURN_V(parser.read() == OK, false);

	// 出于某种原因，Godot 将 `<?xml` 视为 `NODE_UNKNOWN`，并且 `get_node_name` 返回整行内容而不带
	// `<`，这与我在阅读 Godot 的 doctool 时假设的不同。
	// doctool 能够顺利工作是因为它有一个 `NODE_UNKNOWN` 回退，所以没有人注意到这个问题。
	// 参见 https://github.com/godotengine/godot/issues/72517
	//
	// VOXEL_ASSERT_RETURN_V(parser.get_node_type() == XMLParser::NODE_ELEMENT, false);
	// VOXEL_ASSERT_RETURN_V(parser.get_node_name() == "?xml", false);
	// parser.skip_section();
	VOXEL_ASSERT_RETURN_V(parser.read() == OK, false);

	VOXEL_ASSERT_RETURN_V(parser.get_node_type() == XMLParser::NODE_ELEMENT, false);
	VOXEL_ASSERT_RETURN_V(parser.get_node_name() == "nodes", false);

	while (parser.read() == OK) {
		if (parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			if (parser.get_node_name() == "node") {
				VOXEL_ASSERT_RETURN_V(parser.has_attribute("name"), false);
				const String node_name = parser.get_named_attribute_value("name");

				GraphNodeDocumentation *node = find_node_by_name(nodes, node_name);
				if (node == nullptr) {
					VOXEL_PRINT_WARNING("Unknown node name in XML");
					parser.skip_section();

				} else {
					if (parser.has_attribute("category")) {
						node->category = parser.get_named_attribute_value("category");
					}
				}

				while (parser.read() == OK) {
					if (parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						// 我们不解析所有内容，只解析 XML 文件中作为事实来源提供的部分。
						// 其他内容的事实来源来自 C++，稍后才会写入。

						if (parser.get_node_name() == "input") {
							continue;
						}
						if (parser.get_node_name() == "output") {
							continue;
						}
						if (parser.get_node_name() == "parameter") {
							continue;
						}

						if (parser.get_node_name() == "description") {
							VOXEL_ASSERT_RETURN_V(parser.read() == OK, false);

							if (parser.get_node_type() == XMLParser::NODE_TEXT) {
								node->description = parser.get_node_data();
							}

							VOXEL_ASSERT_RETURN_V(parser.read() == OK, false);
							// <description> 结束
							VOXEL_ASSERT_RETURN_V(parser.get_node_type() == XMLParser::NODE_ELEMENT_END, false);

							continue;
						}

						VOXEL_PRINT_WARNING("Unknown XML node");

					} else if (parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
						// <node> 结束
						break;
					}
				}

			} else {
				VOXEL_PRINT_WARNING("Unknown XML node");
			}

		} else if (parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			// <nodes> 结束
			break;
		}
	}

	return true;
}

String strip_eols(const String &text) {
	int pos = 0;
	for (; pos < text.length(); ++pos) {
		const char32_t c = text[pos];
		if (c == '\r' || c == '\n') {
			continue;
		}
		break;
	}
	const int begin = pos;
	for (pos = text.length() - 1; pos >= 0; --pos) {
		const char32_t c = text[pos];
		if (c == '\r' || c == '\n') {
			continue;
		}
		break;
	}
	const int end = pos + 1;
	return text.substr(begin, end - begin);
}

void write_graph_nodes_doc_xml(
		FileAccess &f,
		const StdVector<GraphNodeDocumentation> &nodes_doc,
		const pg::NodeTypeDB &type_db
) {
	class CodeWriter {
	public:
		CodeWriter(FileAccess &f) : _f(f) {}

		void write_line(String s, bool newline = true) {
			for (int i = 0; i < _indent_level; ++i) {
				_f.store_string("\t");
			}
			if (newline) {
				s += "\n";
			}
			_f.store_string(s);
		}

		void indent() {
			++_indent_level;
		}

		void dedent() {
			VOXEL_ASSERT(_indent_level > 0);
			--_indent_level;
		}

		void write_text(String text) {
			String rt = reformat_text(text, _indent_level);
			if (rt.is_empty()) {
				return;
			}
			_f.store_string(rt.xml_escape() + "\n");
		}

	private:
		static int get_indent_level(String s) {
			int space_count = 0;
			int tab_count = 0;
			for (int i = 0; i < s.length(); ++i) {
				const char32_t c = s[i];
				if (c == ' ') {
					++space_count;
				} else if (c == '\t') {
					++tab_count;
				} else {
					break;
				}
			}
			return tab_count + space_count / 4;
		}

		// 文本节点被解析为 `>` 与 `<` 之间的整串字符，这在
		// 回写带缩进的文档时很不方便
		static String reformat_text(String text, int p_indent_level) {
			PackedStringArray lines = text.split("\n");

			while (lines.size() > 0 && lines[0].strip_edges().is_empty()) {
				lines.remove_at(0);
			}

			while (lines.size() > 0 && lines[lines.size() - 1].strip_edges().is_empty()) {
				lines.remove_at(lines.size() - 1);
			}

			{
				// 通过原始指针写入。
				String *lines_p = lines.ptrw();
				for (int line_index = 0; line_index < lines.size(); ++line_index) {
					String s = lines[line_index];

					String indent;
					const int indent_level = get_indent_level(s);
					for (int i = 0; i < indent_level; ++i) {
						indent += "\t";
					}

					lines_p[line_index] = indent + s.strip_edges();
				}
			}

			return String("\n").join(lines);
		}

	private:
		FileAccess &_f;
		int _indent_level = 0;
	};

	CodeWriter w{ f };

	w.write_line("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");

	w.write_line("<nodes>");
	w.indent();

	for (const GraphNodeDocumentation &node_doc : nodes_doc) {
		w.write_line(String("<node name=\"{0}\" category=\"{1}\">").format(varray(node_doc.name, node_doc.category)));
		w.indent();

		pg::VoxelGraphFunction::NodeTypeID type_id;
		VOXEL_ASSERT_RETURN(type_db.try_get_type_id_from_name(node_doc.name, type_id));
		const pg::NodeType &type = type_db.get_type(type_id);

		for (const pg::NodeType::Port &input : type.inputs) {
			w.write_line(String("<input name=\"{0}\" default_value=\"{1}\"/>")
								 .format(varray(input.name, input.default_value)));
		}

		for (const pg::NodeType::Port &output : type.outputs) {
			VOXEL_ASSERT_CONTINUE(!output.name.is_empty());
			if (output.name[0] == '_') {
				// 内部
				continue;
			}
			w.write_line(String("<output name=\"{0}\"/>").format(varray(output.name)));
		}

		for (const pg::NodeType::Param &param : type.params) {
			w.write_line(String("<parameter name=\"{0}\" type=\"{1}\" default_value=\"{2}\"/>")
								 .format(
										 varray(param.name,
												param.type == Variant::OBJECT ? param.class_name
																			  : Variant::get_type_name(param.type),
												param.default_value == Variant() ? "null" : param.default_value)
								 ));
		}

		w.write_line("<description>");
		w.indent();

		w.write_text(node_doc.description);

		w.dedent();
		w.write_line("</description>");

		w.dedent();
		w.write_line("</node>");
	}

	w.dedent();
	w.write_line("</nodes>");
}

// 解析包含图形节点文档的 XML 文件，
// 添加引擎中已知节点的信息，并与 XML 文件中已有的信息合并，
// 移除引擎中已不存在的节点的信息，
// 并将结果写回 XML 文件。
void run_graph_nodes_doc_tool(String src_xml_fpath, String dst_xml_fpath) {
	VOXEL_PRINT_VERBOSE("Running Voxel graph nodes doc tool");

	Ref<XMLParser> parser;
	parser.instantiate();
	{
		const Error err = parser->open(src_xml_fpath);
		if (err != OK) {
			return;
		}
	}

	const pg::NodeTypeDB &type_db = pg::NodeTypeDB::get_singleton();

	StdVector<GraphNodeDocumentation> nodes;

	// 首先用所有已知节点类型填充
	for (int node_type_id = 0; node_type_id < type_db.get_type_count(); ++node_type_id) {
		const pg::NodeType &type = type_db.get_type(node_type_id);

		GraphNodeDocumentation doc;
		doc.name = type.name;

		nodes.push_back(doc);
	}

	// 解析来自 XML 文件的描述和分类
	VOXEL_ASSERT_RETURN(parse_graph_nodes_doc_xml(**parser, nodes));

	struct GraphNodeDocumentationComparer {
		bool operator()(const GraphNodeDocumentation &a, const GraphNodeDocumentation &b) const {
			return a.name < b.name;
		}
	};
	SortArray<GraphNodeDocumentation, GraphNodeDocumentationComparer> sorter;
	sorter.sort(nodes.data(), nodes.size());

	Ref<FileAccess> fa = FileAccess::open(dst_xml_fpath, FileAccess::WRITE);
	VOXEL_ASSERT_RETURN_MSG(fa.is_valid(), "Failed to write XML file");

	// 将合并后的信息写回 XML 文件
	write_graph_nodes_doc_xml(**fa, nodes, type_db);

	VOXEL_PRINT_VERBOSE("Voxel graph nodes doc tool is done.");
}

} // namespace voxel
