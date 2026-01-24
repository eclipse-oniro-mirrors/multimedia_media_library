#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
C++ String Constant Resolver
解析C++头文件中的字符串常量，处理拼接和递归依赖
"""

import re
import os
import sys
from pathlib import Path
from typing import Dict, Set, Optional, Tuple, List


class CppStringConstantResolver:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root).resolve()
        self.constants_cache: Dict[str, str] = {}  # 缓存已解析的常量
        self.processed_files: Set[Path] = set()  # 避免循环依赖

    def parse_file(self, header_file: str) -> Dict[str, str]:
        """
        解析头文件中的字符串常量

        Args:
            header_file: 头文件路径（相对或绝对）

        Returns:
            字典，key为常量名，value为解析后的字面量值
        """
        file_path = self._resolve_path(header_file)
        if not file_path.exists():
            print(f"错误: 文件不存在 {file_path}")
            return {}

        print(f"\n{'='*60}")
        print(f"解析文件: {file_path}")
        print(f"{'='*60}")

        # 清空缓存，开始新的解析
        self.constants_cache.clear()
        self.processed_files.clear()

        return self._parse_file_recursive(file_path)

    def _resolve_path(self, path: str) -> Path:
        """解析文件路径"""
        p = Path(path)
        if p.is_absolute():
            return p
        # 尝试相对于项目根目录
        return (self.project_root / p).resolve()

    def _parse_file_recursive(self, file_path: Path) -> Dict[str, str]:
        """递归解析文件及其依赖"""
        if file_path in self.processed_files:
            return {}

        self.processed_files.add(file_path)

        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
        except Exception as e:
            print(f"警告: 无法读取文件 {file_path}: {e}")
            return {}

        # 解析include依赖
        includes = self._extract_includes(content, file_path)

        # 先解析依赖文件
        for inc_path in includes:
            self._parse_file_recursive(inc_path)

        # 解析当前文件的常量
        constants = self._extract_constants(content, file_path)

        return constants

    def _remove_comments(self, content: str) -> str:
        """移除C++注释"""
        # 移除单行注释
        content = re.sub(r'//.*?$', '', content, flags=re.MULTILINE)
        # 移除多行注释
        content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
        return content

    def _extract_includes(self, content: str, current_file: Path) -> List[Path]:
        """提取#include依赖"""
        includes = []
        # 匹配 #include "xxx.h" 和 #include <xxx.h>
        pattern = r'#include\s+[<"]([^>"]+)[>"]'

        for match in re.finditer(pattern, content):
            inc_file = match.group(1)
            inc_path = self._find_include_file(inc_file, current_file)
            if inc_path:
                includes.append(inc_path)

        return includes

    def _find_include_file(self, include_name: str, current_file: Path) -> Optional[Path]:
        """查找include文件的实际路径"""
        # 1. 相对于当前文件的目录
        candidate = current_file.parent / include_name
        if candidate.exists():
            return candidate.resolve()

        # 2. 相对于项目根目录搜索
        for path in self.project_root.rglob(include_name):
            if path.is_file():
                return path.resolve()

        return None

    def _extract_constants(self, content: str, file_path: Path) -> Dict[str, str]:
        """提取文件中的字符串常量定义"""
        constants = {}

        # 移除注释
        #content = self._remove_comments(content)

        # 匹配 const std::string 和 inline const std::string
        # 支持多行定义
        pattern = r'(?:inline\s+)?const\s+std::string\s+(\w+)\s*=\s*([^;]+);'
        # pattern = r'(?:inline\s+)?const\s+std::string\s+(\w+)\s*=\s*\"([^\"]+)\";'

        for match in re.finditer(pattern, content, re.MULTILINE | re.DOTALL):
            const_name = match.group(1)
            const_value = match.group(2).strip()

            # 解析常量值
            resolved_value = self._resolve_constant_value(const_value, file_path)

            constants[const_name] = resolved_value
            self.constants_cache[const_name] = resolved_value

            print(f"\n常量: {const_name}")
            print(f"  原始定义: {const_value[:100]}{'...' if len(const_value) > 100 else ''}")
            print(f"  解析结果: {resolved_value}")
        return constants

    def _resolve_constant_value(self, value: str, file_path: Path) -> str:
        """
        解析常量值，处理字符串拼接和常量引用

        Args:
            value: 常量定义的右值
            file_path: 当前文件路径（用于调试）

        Returns:
            解析后的字符串字面量
        """
        # 清理空白字符，但保留字符串内的空白
        value = value.strip()

        # 处理字符串拼接（可能跨多行）
        # 提取所有的字符串字面量和常量引用
        tokens = self._tokenize_string_expression(value)

        # 解析每个token
        result_parts = []
        for token in tokens:
            resolved = self._resolve_token(token)
            if resolved is not None:
                result_parts.append(resolved)

        return ''.join(result_parts)

    def _tokenize_string_expression(self, expr: str) -> List[str]:
        """
        将字符串表达式分解为token列表
        支持: "literal" + CONSTANT + "literal"
        """
        tokens = []
        expr = expr.strip()

        i = 0
        while i < len(expr):
            # 跳过空白
            while i < len(expr) and expr[i].isspace():
                i += 1

            if i >= len(expr):
                break

            # 字符串字面量 "..." 或 R"(...)"
            if expr[i] == '"':
                token, end = self._extract_string_literal(expr, i)
                tokens.append(token)
                i = end
            elif expr[i:i+2] == 'R"':
                token, end = self._extract_raw_string_literal(expr, i)
                tokens.append(token)
                i = end
            # 加号（跳过）
            elif expr[i] == '+':
                i += 1
            # 常量名
            elif expr[i].isalpha() or expr[i] == '_':
                token, end = self._extract_identifier(expr, i)
                tokens.append(token)
                i = end
            else:
                i += 1

        return tokens

    def _extract_string_literal(self, expr: str, start: int) -> Tuple[str, int]:
        """提取普通字符串字面量，返回带引号标记的元组"""
        i = start + 1  # 跳过开始的引号
        chars = []

        while i < len(expr):
            if expr[i] == '\\' and i + 1 < len(expr):
                # 处理转义字符
                next_char = expr[i + 1]
                if next_char == 'n':
                    chars.append('\n')
                elif next_char == 't':
                    chars.append('\t')
                elif next_char == 'r':
                    chars.append('\r')
                elif next_char == '\\':
                    chars.append('\\')
                elif next_char == '"':
                    chars.append('"')
                elif next_char == '\'':
                    chars.append('\'')
                elif next_char == '0':
                    chars.append('\0')
                else:
                    # 其他转义字符保持原样
                    chars.append(next_char)
                i += 2
            elif expr[i] == '"':
                # 字符串结束，返回时添加特殊标记表示这是字面量
                return (('LITERAL:', ''.join(chars)), i + 1)
            else:
                chars.append(expr[i])
                i += 1

        # 未闭合的字符串
        return (('LITERAL:', ''.join(chars)), i)

    def _extract_raw_string_literal(self, expr: str, start: int) -> Tuple[str, int]:
        """提取原始字符串字面量 R"delimiter(content)delimiter" """
        # R"delimiter(
        i = start + 2  # 跳过 R"

        # 提取delimiter
        delimiter_start = i
        while i < len(expr) and expr[i] != '(':
            i += 1

        if i >= len(expr):
            return (('LITERAL:', ''), i)

        delimiter = expr[delimiter_start:i]
        i += 1  # 跳过 (

        # 提取内容直到 )delimiter"
        end_marker = ')' + delimiter + '"'
        content_start = i

        end_pos = expr.find(end_marker, i)
        if end_pos == -1:
            # 未找到结束标记
            return (('LITERAL:', expr[content_start:]), len(expr))

        content = expr[content_start:end_pos]
        return (('LITERAL:', content), end_pos + len(end_marker))

    def _extract_identifier(self, expr: str, start: int) -> Tuple[str, int]:
        """提取标识符（常量名），返回带常量标记的元组"""
        i = start
        while i < len(expr) and (expr[i].isalnum() or expr[i] == '_' or expr[i] == ':'):
            i += 1
        return (('CONSTANT:', expr[start:i]), i)

    def _resolve_token(self, token: str) -> Optional[str]:
        """
        解析单个token
        - 如果是字符串字面量（带LITERAL:标记），直接返回内容
        - 如果是常量名（带CONSTANT:标记），从缓存中查找并递归解析
        """
        if not token:
            return None

        # 检查token类型
        if isinstance(token, tuple) and len(token) == 2:
            token_type, token_value = token

            if token_type == 'LITERAL:':
                # 字符串字面量，直接返回
                return token_value
            elif token_type == 'CONSTANT:':
                # 常量引用，需要查找并解析
                token_value = token_value.strip()
                # 处理命名空间限定符 (如 namespace::CONSTANT)
                const_name = token_value.split('::')[-1]

                # 从缓存中查找
                if const_name in self.constants_cache:
                    return self.constants_cache[const_name]
                else:
                    # 未找到常量定义，返回空字符串
                    print(f"  警告: 未找到常量 '{token_value}' 的定义，跳过")
                    return ""

        # 兼容旧格式（字符串）
        token_str = str(token).strip()
        if not token_str:
            return None

        # 如果看起来像常量名（不包含引号）
        if '"' not in token_str and "'" not in token_str:
            const_name = token_str.split('::')[-1]
            if const_name in self.constants_cache:
                return self.constants_cache[const_name]
            else:
                print(f"  警告: 未找到常量 '{token_str}' 的定义，跳过")
                return ""

        # 否则认为是字符串字面量
        return token_str


def print_usage():
    """打印使用说明"""
    print("""
使用方法:
    python cpp_string_constant_resolver.py <项目根目录> <头文件路径> [--macro]

参数:
    项目根目录: C++项目的根目录路径
    头文件路径: 要解析的.h头文件路径（可以是相对或绝对路径）
    --macro: 可选参数，如果指定则额外生成宏定义版本的文件

示例:
    # 生成展开后的备份文件
    python cpp_string_constant_resolver.py /path/to/project include/constants.h

    # 同时生成备份文件和宏定义文件
    python cpp_string_constant_resolver.py . services/media_analysis_data_manager/include/dao/table/vision_db_sqls.h --macro
""")


def generate_backup_file(original_file: Path, constants: Dict[str, str]) -> Path:
    """
    生成备份文件，将常量替换为展开后的字面量

    Args:
        original_file: 原始头文件路径
        constants: 解析后的常量字典

    Returns:
        生成的备份文件路径
    """
    # 读取原始文件
    with open(original_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # 生成备份文件名
    file_stem = original_file.stem  # 不带扩展名的文件名
    file_suffix = original_file.suffix  # 扩展名
    backup_file = original_file.parent / f"{file_stem}_bak{file_suffix}"

    # 替换常量定义
    modified_content = content

    # 匹配并替换每个常量定义
    for const_name, const_value in constants.items():
        # 匹配模式：(inline )?const std::string CONST_NAME = ...;
        pattern = r'((?:inline\s+)?const\s+std::string\s+' + re.escape(const_name) + r'\s*=\s*)([^;]+)(;)'

        def replace_func(match):
            prefix = match.group(1)  # const std::string CONST_NAME =
            suffix = match.group(3)  # ;
            # 转义字符串中的特殊字符
            escaped_value = const_value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n').replace('\t', '\\t').replace('\r', '\\r')
            return f'{prefix}"{escaped_value}"{suffix}'

        modified_content = re.sub(pattern, replace_func, modified_content, flags=re.MULTILINE | re.DOTALL)

    # 写入备份文件
    with open(backup_file, 'w', encoding='utf-8') as f:
        f.write(modified_content)

    return backup_file


def generate_macro_file(original_file: Path, constants: Dict[str, str]) -> Path:
    """
    生成宏定义文件，将字符串常量转换为宏定义

    Args:
        original_file: 原始头文件路径
        constants: 解析后的常量字典

    Returns:
        生成的宏定义文件路径
    """
    # 读取原始文件
    with open(original_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # 生成宏定义文件名
    file_stem = original_file.stem  # 不带扩展名的文件名
    file_suffix = original_file.suffix  # 扩展名
    macro_file = original_file.parent / f"{file_stem}_macro{file_suffix}"

    # 替换常量定义为宏定义
    modified_content = content

    # 匹配并替换每个常量定义
    for const_name, const_value in constants.items():
        # 匹配模式：(inline )?const std::string CONST_NAME = ...;
        pattern = r'(?:inline\s+)?const\s+std::string\s+' + re.escape(const_name) + r'\s*=\s*[^;]+;'

        def replace_func(match):
            # 转义字符串中的特殊字符（但不转义已经存在的转义序列）
            escaped_value = const_value.replace('\\', '\\\\').replace('"', '\\"')

            # 检查字符串长度，如果太长需要分行
            max_line_length = 12000# 每行最大长度

            # 计算 #define NAME " 的长度
            define_prefix = f'#define {const_name} "'
            define_prefix_len = len(define_prefix)

            # 如果整个定义可以放在一行
            if define_prefix_len + len(escaped_value) + 1 <= max_line_length:
                return f'#define {const_name} "{escaped_value}"'

            # 需要分行处理
            lines = []
            indent = '    '  # 续行缩进

            # 智能分割：在合适的位置断开（空格、逗号、括号等）
            def smart_split(text, max_len):
                """智能分割字符串，尽量在合适的位置断开"""
                if len(text) <= max_len:
                    return [text]

                result = []
                current = ""

                # 优先在这些字符后断开
                break_chars = [' ', ',', '(', ')', '{', '}', ';']

                i = 0
                while i < len(text):
                    char = text[i]

                    # 如果加上当前字符会超长
                    if len(current) + 1 > max_len:
                        # 尝试回退到最近的断点
                        if current:
                            # 从后往前找合适的断点
                            break_pos = -1
                            for j in range(len(current) - 1, max(0, len(current) - 20), -1):
                                if current[j] in break_chars:
                                    break_pos = j + 1
                                    break

                            if break_pos > 0 and break_pos < len(current):
                                # 在断点处分割
                                result.append(current[:break_pos])
                                current = current[break_pos:]
                            else:
                                # 没找到合适的断点，强制分割
                                result.append(current)
                                current = ""

                    current += char
                    i += 1

                if current:
                    result.append(current)

                return result

            # 第一行可用长度
            first_line_max = max_line_length - define_prefix_len - 3  # 留出 " \
            # 后续行可用长度
            cont_line_max = max_line_length - len(indent) - 3  # 留出 " \

            # 分割字符串
            chunks = []
            remaining = escaped_value

            # 第一块
            if len(remaining) <= first_line_max:
                chunks.append(remaining)
            else:
                # 智能分割第一行
                first_parts = smart_split(remaining, first_line_max)
                if first_parts:
                    chunks.append(first_parts[0])
                    remaining = remaining[len(first_parts[0]):]

                    # 分割剩余部分
                    while remaining:
                        parts = smart_split(remaining, cont_line_max)
                        if parts:
                            chunks.append(parts[0])
                            remaining = remaining[len(parts[0]):]
                        else:
                            break

            # 生成多行宏定义
            if len(chunks) == 1:
                return f'#define {const_name} "{chunks[0]}"'
            else:
                # 第一行
                lines.append(f'#define {const_name} "{chunks[0]}" \\')

                # 中间行
                for i in range(1, len(chunks) - 1):
                    lines.append(f'{indent}"{chunks[i]}" \\')

                # 最后一行（不需要反斜杠）
                lines.append(f'{indent}"{chunks[-1]}"')

                return '\n'.join(lines)

        modified_content = re.sub(pattern, replace_func, modified_content, flags=re.MULTILINE | re.DOTALL)

    # 写入宏定义文件
    with open(macro_file, 'w', encoding='utf-8') as f:
        f.write(modified_content)

    return macro_file

    return macro_file


def main():
    """主函数"""
    if len(sys.argv) < 3:
        print("错误: 参数数量不正确")
        print_usage()
        sys.exit(1)

    project_root = sys.argv[1]
    header_file = sys.argv[2]
    generate_macro = False

    # 检查是否有 --macro 参数
    if len(sys.argv) > 3 and sys.argv[3] == '--macro':
        generate_macro = True

    if not os.path.exists(project_root):
        print(f"错误: 项目根目录不存在: {project_root}")
        sys.exit(1)

    resolver = CppStringConstantResolver(project_root)
    constants = resolver.parse_file(header_file)

    print(f"\n{'='*60}")
    print(f"解析完成，共找到 {len(resolver.constants_cache)} 个常量")
    print(f"{'='*60}\n")

    # 输出汇总
    for name, value in sorted(resolver.constants_cache.items()):
        print(f"{name} = \"{value}\"")

    # 生成备份文件
    if constants:
        original_path = resolver._resolve_path(header_file)
        backup_path = generate_backup_file(original_path, constants)
        print(f"\n{'='*60}")
        print(f"已生成备份文件: {backup_path}")
        print(f"{'='*60}")

        # 如果指定了 --macro 参数，生成宏定义文件
        if generate_macro:
            macro_path = generate_macro_file(original_path, constants)
            print(f"\n{'='*60}")
            print(f"已生成宏定义文件: {macro_path}")
            print(f"{'='*60}")
    else:
        print("\n未找到任何常量，不生成备份文件")


if __name__ == "__main__":
    main()

