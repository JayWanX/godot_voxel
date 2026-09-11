#ifndef VOXEL_GODOT_NODE_BUCKETS_STRATEGY_H
#define VOXEL_GODOT_NODE_BUCKETS_STRATEGY_H

#include "../containers/std_vector.h"
#include "../errors.h"
#include "macros.h"

class Node;

namespace voxel::godot {

// 这是对 Godot 的一个变通：当节点有大量兄弟节点时，从场景树中移除节点非常慢……
// 参见 https://github.com/godotengine/godot/issues/61929
template <typename TBucket>
class NodeBucketsStrategy {
public:
	static const unsigned int BUCKET_SIZE = 20;

	NodeBucketsStrategy(Node &parent) : _parent(parent) {}

	void add_child(Node *node) {
		VOXEL_ASSERT(node != nullptr);
		if (_free_buckets.size() == 0) {
			TBucket *bucket = memnew(TBucket);
			_used_buckets.push_back(bucket);
			bucket->add_child(node);
			_parent.add_child(bucket);
		} else {
			TBucket *bucket = _free_buckets.back();
			_free_buckets.pop_back();
			_used_buckets.push_back(bucket);
			bucket->add_child(node);
		}
	}

	void remove_empty_buckets() {
		for (unsigned int i = 0; i < _used_buckets.size();) {
			TBucket *bucket = _used_buckets[i];
			if (bucket->get_child_count() == 0) {
				bucket->queue_free();
				_used_buckets[i] = _used_buckets.back();
				_used_buckets.pop_back();
			} else {
				++i;
			}
		}
	}

private:
	Node &_parent;
	StdVector<TBucket *> _free_buckets;
	StdVector<TBucket *> _used_buckets;
};

} // namespace voxel::godot

#endif // VOXEL_GODOT_NODE_BUCKETS_STRATEGY_H
