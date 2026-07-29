#!/usr/bin/env python3
"""Generate Clay's editor APIs from Doxygen comments in extension_clay.cpp.

The workflow is adapted from https://github.com/defold/extension-rive. It emits
both the native Defold ``.script_api`` format and richer LuaLS-style annotations.
"""

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "clay/src/extension_clay.cpp"
OUTPUT_SCRIPT_API = ROOT / "clay/api/clay.script_api"
OUTPUT_LUA = ROOT / "clay/api/clay.lua"

COMMENT_PATTERN = re.compile(r"/\*\*(.*?)\*/", re.S)
TYPED_PATTERN = re.compile(
    r"(?P<name>\[[^\]]+\]|[^\s]+)\s+\[type:\s*(?P<type>[^\]]+)\]\s*(?P<desc>.*)"
)


def comment_lines(comment: str) -> list[str]:
    lines = []
    for raw in comment.splitlines():
        line = raw.strip()
        if line.startswith("*"):
            line = line[1:].lstrip()
        lines.append(line)
    return lines


def parse_typed(text: str, context: str) -> dict:
    match = TYPED_PATTERN.fullmatch(text.strip())
    if not match:
        raise SystemExit(f"Could not parse {context}: {text!r}")
    return {
        "name": match.group("name"),
        "type": match.group("type").strip(),
        "desc": match.group("desc").strip(),
    }


def description_before_tags(lines: list[str]) -> str:
    description = []
    for line in lines:
        if line.startswith("@"):
            break
        if line:
            description.append(line)
    return " ".join(description)


def parse_function_doc(lines: list[str]) -> dict:
    result = {
        "description": description_before_tags(lines),
        "parameters": [],
        "returns": [],
    }
    for line in lines:
        if line.startswith("@param "):
            result["parameters"].append(parse_typed(line[len("@param ") :], "parameter"))
        elif line.startswith("@return "):
            result["returns"].append(parse_typed(line[len("@return ") :], "return"))
    return result


def parse_source(text: str) -> tuple[dict, list[dict], list[dict], dict]:
    module = None
    aliases = []
    classes = []
    functions = {}

    for match in COMMENT_PATTERN.finditer(text):
        lines = comment_lines(match.group(1))

        module_line = next((line for line in lines if line.startswith("@module ")), None)
        if module_line:
            module = {
                "name": module_line[len("@module ") :].strip(),
                "description": description_before_tags(lines),
                "constants": [],
            }
            for line in lines:
                if line.startswith("@constant "):
                    module["constants"].append(parse_typed(line[len("@constant ") :], "constant"))
            continue

        alias_line = next((line for line in lines if line.startswith("@alias ")), None)
        if alias_line:
            alias = parse_typed(alias_line[len("@alias ") :], "alias")
            alias["values"] = [
                line[len("@value ") :].strip()
                for line in lines
                if line.startswith("@value ")
            ]
            aliases.append(alias)
            continue

        class_line = next((line for line in lines if line.startswith("@class ")), None)
        if class_line:
            class_name = class_line[len("@class ") :].strip()
            class_index = lines.index(class_line)
            description_lines = []
            for line in lines[class_index + 1 :]:
                if line.startswith("@"):
                    break
                if line:
                    description_lines.append(line)
            classes.append(
                {
                    "name": class_name,
                    "description": " ".join(description_lines),
                    "fields": [
                        parse_typed(line[len("@field ") :], f"field on {class_name}")
                        for line in lines
                        if line.startswith("@field ")
                    ],
                }
            )
            continue

        following = text[match.end() :]
        function_match = re.match(r"\s*static int (dclay_\w+)\s*\(", following)
        if function_match:
            functions[function_match.group(1)] = parse_function_doc(lines)

    if not module:
        raise SystemExit("No @module documentation found.")
    return module, aliases, classes, functions


def parse_registration(text: str) -> list[tuple[str, str]]:
    match = re.search(r"static const luaL_reg Module_methods\[\]\s*=\s*\{(.*?)\};", text, re.S)
    if not match:
        raise SystemExit("Module_methods registration table not found.")
    return re.findall(r'\{\s*"([^"]+)"\s*,\s*(dclay_\w+)\s*\}', match.group(1))


def parse_registered_constants(text: str) -> list[str]:
    match = re.search(r"static void init_lua\(lua_State\* L\)(.*?)static dmExtension::Result", text, re.S)
    if not match:
        raise SystemExit("init_lua() not found.")
    return re.findall(r'lua_setfield\(L, -2, "([A-Z][A-Z0-9_]+)"\)', match.group(1))


def without_nil(type_name: str) -> tuple[str, bool]:
    parts = [part.strip() for part in type_name.split("|")]
    optional = "nil" in parts
    return "|".join(part for part in parts if part != "nil"), optional


def quoted(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def validate(module: dict, registrations: list[tuple[str, str]], functions: dict, registered_constants: list[str]):
    registered_symbols = {symbol for _, symbol in registrations}
    missing_docs = sorted(registered_symbols - functions.keys())
    stale_docs = sorted(functions.keys() - registered_symbols)
    if missing_docs or stale_docs:
        raise SystemExit(
            f"Function documentation mismatch: missing={missing_docs}, stale={stale_docs}"
        )

    documented_constants = [entry["name"] for entry in module["constants"]]
    if len(documented_constants) != len(set(documented_constants)):
        raise SystemExit("Duplicate constant documentation found.")
    missing_constants = [name for name in registered_constants if name not in documented_constants]
    stale_constants = [name for name in documented_constants if name not in registered_constants]
    if missing_constants or stale_constants:
        raise SystemExit(
            "Constant documentation mismatch: "
            f"missing={missing_constants}, stale={stale_constants}"
        )


def write_script_api(module: dict, members: list[dict]):
    lines = [
        "# Auto generated from utils/update_script_api.py. Do not edit manually.",
        f"- name: {module['name']}",
        "  type: table",
        f"  desc: {quoted(module['description'])}",
        "  members:",
    ]

    for member in members:
        lines.extend(
            [
                f"  - name: {member['name']}",
                "    type: function",
                f"    desc: {quoted(member['description'])}",
            ]
        )
        if member["parameters"]:
            lines.append("    parameters:")
            for param in member["parameters"]:
                type_name, optional = without_nil(param["type"])
                lines.extend(
                    [
                        f"    - name: {param['name']}",
                        f"      type: {quoted(type_name)}",
                        f"      desc: {quoted(param['desc'])}",
                    ]
                )
                if optional:
                    lines.append("      optional: true")
        if member["returns"]:
            lines.append("    returns:")
            for value in member["returns"]:
                lines.extend(
                    [
                        f"    - name: {value['name']}",
                        f"      type: {quoted(value['type'])}",
                        f"      desc: {quoted(value['desc'])}",
                    ]
                )
        lines.append("")

    for constant in module["constants"]:
        lines.extend(
            [
                f"  - name: {constant['name']}",
                "    type: constant",
                f"    desc: {quoted(constant['desc'])}",
                "",
            ]
        )

    OUTPUT_SCRIPT_API.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_SCRIPT_API.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def write_lua(module: dict, aliases: list[dict], classes: list[dict], members: list[dict]):
    lines = [
        "-- Auto generated from utils/update_script_api.py.",
        "-- WARNING: Do not edit manually. Update the Doxygen comments in extension_clay.cpp.",
        "",
        "---@meta",
        "---@diagnostic disable: lowercase-global",
        "---@diagnostic disable: missing-return",
        "---@diagnostic disable: duplicate-doc-param",
        "---@diagnostic disable: duplicate-set-field",
        "---@diagnostic disable: args-after-dots",
        "",
        "---@class defold_api.clay",
    ]

    for constant in module["constants"]:
        if constant["desc"]:
            lines.append(f"---{constant['desc']}")
        lines.append(f"---@field {constant['name']} {constant['type']}")
    lines.extend(["clay = {}", ""])

    for alias in aliases:
        if alias["desc"]:
            lines.append(f"---{alias['desc']}")
        lines.append(f"---@alias {alias['name']} {alias['type']}")
        lines.extend(f"---| `{value}`" for value in alias["values"])
        lines.append("")

    for class_info in classes:
        if class_info["description"]:
            lines.append(f"---{class_info['description']}")
        lines.append(f"---@class {class_info['name']}")
        for field in class_info["fields"]:
            type_name, optional = without_nil(field["type"])
            field_name = field["name"] + ("?" if optional and not field["name"].startswith("[") else "")
            suffix = f" {field['desc']}" if field["desc"] else ""
            lines.append(f"---@field {field_name} {type_name}{suffix}")
        lines.append("")

    for member in members:
        lines.append(f"---{member['description']}")
        for param in member["parameters"]:
            type_name, optional = without_nil(param["type"])
            param_name = param["name"] + ("?" if optional else "")
            suffix = f" {param['desc']}" if param["desc"] else ""
            lines.append(f"---@param {param_name} {type_name}{suffix}")
        for value in member["returns"]:
            suffix = f" {value['desc']}" if value["desc"] else ""
            lines.append(f"---@return {value['type']} {value['name']}{suffix}")
        params = ", ".join(param["name"] for param in member["parameters"])
        lines.extend([f"function clay.{member['name']}({params}) end", ""])

    OUTPUT_LUA.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main():
    text = SOURCE.read_text(encoding="utf-8")
    module, aliases, classes, function_docs = parse_source(text)
    registrations = parse_registration(text)
    registered_constants = parse_registered_constants(text)
    validate(module, registrations, function_docs, registered_constants)

    members = []
    for lua_name, symbol in registrations:
        member = dict(function_docs[symbol])
        member["name"] = lua_name
        members.append(member)

    write_script_api(module, members)
    write_lua(module, aliases, classes, members)
    print(
        f"Generated {len(members)} functions, {len(module['constants'])} constants, "
        f"{len(classes)} classes, and {len(aliases)} aliases."
    )


if __name__ == "__main__":
    main()
