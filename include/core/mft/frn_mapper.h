#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

/*
 * FRNMapper
 * ------------------------------------------------
 * 职责：
 * 1. 维护 FRN → 父 FRN 的映射
 * 2. 支持路径回溯
 * 3. 处理硬链接 / 多父情况
 */
class FRNMapper {
public:
    struct Node {
        uint64_t frn;
        std::vector<uint64_t> parent_frns;
        std::wstring name;
        bool is_directory;
    };

public:
    FRNMapper() = default;

    // 插入 / 更新节点
    void add_node(const Node& node);

    // 删除节点
    void remove_node(uint64_t frn);

    // 构建完整路径
    std::wstring build_path(uint64_t frn) const;

    // 是否存在
    bool exists(uint64_t frn) const;

private:
    std::wstring build_recursive(
        uint64_t frn,
        std::unordered_set<uint64_t>& visited
    ) const;

private:
    std::unordered_map<uint64_t, Node> m_nodes;
};