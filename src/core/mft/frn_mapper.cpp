#include "core/mft/frn_mapper.h"

#include <unordered_set>

void FRNMapper::add_node(const Node& node) {
    auto& dst = m_nodes[node.frn];

    // 第一次插入
    if (dst.frn == 0) {
        dst.frn = node.frn;
        dst.name = node.name;
        dst.is_directory = node.is_directory;
    }

    // 合并 parent（支持硬链接）
    for (uint64_t parent : node.parent_frns) {
        bool exists = false;
        for (uint64_t p : dst.parent_frns) {
            if (p == parent) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            dst.parent_frns.push_back(parent);
        }
    }
}

void FRNMapper::remove_node(uint64_t frn) {
    m_nodes.erase(frn);

    // 同时从其他节点的 parent_frns 中移除
    for (auto& [_, node] : m_nodes) {
        auto& parents = node.parent_frns;
        parents.erase(
            std::remove(parents.begin(), parents.end(), frn),
            parents.end()
        );
    }
}

bool FRNMapper::exists(uint64_t frn) const {
    return m_nodes.find(frn) != m_nodes.end();
}

std::wstring FRNMapper::build_path(uint64_t frn) const {
    std::unordered_set<uint64_t> visited;
    return build_recursive(frn, visited);
}

std::wstring FRNMapper::build_recursive(
    uint64_t frn,
    std::unordered_set<uint64_t>& visited
) const {
    auto it = m_nodes.find(frn);
    if (it == m_nodes.end()) {
        return L"";  // 父不存在（孤儿）
    }

    // 防止循环（NTFS 异常 / USN 未同步）
    if (visited.count(frn)) {
        return L"";
    }
    visited.insert(frn);

    const Node& node = it->second;

    // 根目录（没有 parent）
    if (node.parent_frns.empty()) {
        return node.name;
    }

    // Everything 行为：只取第一个 parent（主路径）
    uint64_t parent = node.parent_frns[0];

    std::wstring parent_path = build_recursive(parent, visited);
    if (parent_path.empty()) {
        return node.name;
    }

    // 拼接路径
    if (parent_path.back() == L'\\') {
        return parent_path + node.name;
    } else {
        return parent_path + L"\\" + node.name;
    }
}
