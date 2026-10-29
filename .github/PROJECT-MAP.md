# PROJECT-MAP — <ProjectName>

> **Purpose**: the single project-specific entry point for every agent and skill in `.github/` and `.claude/skills/`.
> Agents and skills are project-agnostic; before doing any work they read this file to learn what the project is,
> where its modules and docs live, how to build, and how to write commit messages.
>
> Fill in every `<...>` below. Delete the example rows.

---

## 1. Project Overview

Homework 是一個作業實作專案（Training），使用 C++ 開發，以 Visual Studio 解決方案 `Homework.sln` 管理。
建置組態為 Release / x64。

## 2. Module Map

Use the source paths you are working on to resolve the module, its commit scope, and which docs to read.

| Commit scope | Source paths | Docs to read | Notes |
|---|---|---|---|
| `Training` | `**` | — | 單一模組，所有程式碼都使用此 scope |

## 3. Build Commands

| Target | Command |
|---|---|
| Homework | `msbuild Homework.sln /p:Configuration=Release /p:Platform=x64` |

## 4. Commit Message Convention

```
[scope][Feature/Fix/Modify] concise description
```

| Field | Values |
|---|---|
| **scope** | <list valid scopes>|
| **Type** | `Feature` (new feature), `Fix` (bug fix), `Modify` (Adjust existing logic or UI)|

Examples:


- `[Training][Feature] Initial project`
- `[Training][Feature] Add xxx feature`
- `[Training][Feature] Support xxx feature`
- `[Training][Modify] Modify xxx logic`
- `[Training][Fix] Fix xxx bug`
- `[Training][Release] Release V1.0.0.0`

Pick the **scope** that best matches the changed files (see Module Map above).

## 5. Code Conventions

None yet.

## 6. Code Review — Project-Specific High-Risk Patterns

When reviewing C/C++ or C# diffs, in addition to the generic review skill, load:

- None yet.
- **Security knowledge base**: past internal security incidents, as `KB-*.md` entries. Config: [`security-kb.json`](security-kb.json) (local copy in `.kb/`, git-ignored). Run [`scripts/Find-SecurityKbHits.ps1`](scripts/Find-SecurityKbHits.ps1) on the diff and read each entry it lists. The rules are in the **Security Knowledge Base** section of [`agents/reviewer.agent.md`](agents/reviewer.agent.md).

## 7. Feature Areas

N/A.
