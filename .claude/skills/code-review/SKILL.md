---
name: code-review
description: GlideX code review flow for Claude Code (port of the Copilot Reviewer / ReviewerCodex / Planner / Executor agent chain). Reviews the latest commit (default) or uncommitted changes against the project review skills, confirms each finding with the user (Accept/Dismiss), plans and applies fixes for confirmed findings, and stamps the commit with `[copilot-reviewed]` when the review passes. Use when the user says review, review commit, review latest commit, review uncommitted, cross-check, second-pass, glidex review, 審查, 幫我 review, review 一下.
argument-hint: "[commit | uncommitted | cross-check]"
allowed-tools: Read, Grep, Glob, Agent, AskUserQuestion, Bash(git log:*), Bash(git show:*), Bash(git diff:*), Bash(git status:*), Bash(git rev-parse:*), Bash(grep:*), PowerShell(pwsh -NoProfile -File .github/scripts/Find-SecurityKbHits.ps1:*)
---

# GlideX Review (Claude Code)

**Source of truth.** This skill reproduces the *flow* of `.github/agents/reviewer.agent.md`. The *rules* are not copied here — you read them from the repo at run time (step 2), so Copilot and Claude always review against the same checklists. If the flow changes, update the agent file first, then this skill.

You are a **senior code reviewer**. Steps 1–6 are read-only: do not edit files, stage, or commit. Writing happens only in step 7, and only after the user approves it.

Before anything else, `Read` `.github/PROJECT-MAP.md` (module map, build commands, §4 commit convention, §5 code conventions, §6 high-risk patterns doc).

## Copilot → Claude Code mapping

| Copilot agent | Claude Code |
|---|---|
| `runInTerminal` | `Bash` (Git Bash). Use the `PowerShell` tool only for the PowerShell snippet in the agent file. |
| `readFile` | `Read` |
| `get_changed_files` | `git status --porcelain` + `git diff HEAD`, then `Read` each untracked (`??`) file |
| `Explore` subagent | `Agent` with `subagent_type: "Explore"` (run independent ones in parallel in one message) |
| `vscode/askQuestions` | `AskUserQuestion`: header ≤ 12 chars, 2–4 options. "Other" is always available, so free-text input is built in. |
| Handoff **Plan Fixes** → Planner → Executor | Step 7 **Fix loop** (done in this session) |
| Handoff **Codex Cross-Check** → ReviewerCodex | Step 7: the user runs ReviewerCodex in VS Code Copilot Chat. Codex is not available in Claude Code. |

---

## 1. Mode and diff

Pick the mode from the skill argument or the user's wording:

| Mode | Trigger | Get the diff |
|---|---|---|
| **Uncommitted** | `uncommitted`, working tree, staged, 未 commit | `git status --porcelain`, `git diff HEAD`, `Read` untracked files. Tell the user you are reviewing uncommitted changes. |
| **Second-pass** | `cross-check`, `second-pass`, "after Codex" | `git log -1`, `git show --name-only HEAD`, `git show HEAD` |
| **First-pass** (default) | anything else | same as second-pass |

Do not mix the uncommitted and commit methods. If the diff is empty, say so and stop.

## 2. Load the review rules (MANDATORY)

`Read` every matching file **before** step 3. One file can match several rows, so load all of them.

| Changed file | Load |
|---|---|
| C/C++ `.c .cc .cpp .cxx .h .hh .hpp` | `.github/skills/cpp-code-review-guidelines/SKILL.md` |
| C# `.cs .csproj` | `.github/skills/csharp-code-review-guidelines/SKILL.md` |
| Driver: under `asextdisplay/`, `GlideXBusEnumerator/`, `GlideXPenDevice/`, `GlideXSharedAudio/`, `asbtinput/`; or `DriverIOCTL.*`, `DriverHelper.*`, `DriverInstaller.*`, `HIDTouchInfo.h`, `HIDMouseInfo.h`, `HIDKeyboardInfo.h`; or any `.inf` / `.inx` | `.github/skills/virtual-driver-review-guidelines/SKILL.md` **plus** the C/C++ skill |
| XAML `.xaml` | C# skill + the **XAML** section of the high-risk patterns doc |
| Docs `.md` | **Documentation Review Checklist** in the agent file |

**Always** also:

1. `Read` the high-risk patterns doc linked from `PROJECT-MAP.md` §6 (currently `.github/docs/Windows/glidex-review-patterns.md`).
2. `Read` `.github/agents/reviewer.agent.md` sections **Documentation Review Checklist**, **Project Conventions Checklist**, **Commit Message Convention** and **Scope Rules**. Apply Project Conventions to every code file (`.c/.cpp/.h/.cs/.xaml/.idl`) and the commit-message check to the commit (commit modes only).
3. Run the non-ASCII check (Project Conventions #1). Bash form:
   ```bash
   git show HEAD --format= -- '*.c' '*.cpp' '*.h' | grep -P '^\+\+\+ |^\+(?!\+\+ ).*[^\x00-\x7F]'
   ```
   In uncommitted mode, replace `git show HEAD --format=` with `git diff HEAD`. Untracked `.c/.cpp/.h` files are entirely new, so check them with `grep -nP '[^\x00-\x7F]' <file>`. Apply the exemptions listed in the agent file (BOM, vendored or IDE-generated new files).
4. **Security KB.** Follow the **Security Knowledge Base** section of the agent file. Run the matcher with the `PowerShell` tool:
   ```powershell
   pwsh -NoProfile -File .github/scripts/Find-SecurityKbHits.ps1 -Mode commit -Update       # first-pass / second-pass
   pwsh -NoProfile -File .github/scripts/Find-SecurityKbHits.ps1 -Mode uncommitted -Update  # uncommitted
   ```
   `Read` every entry file it lists. If it prints a warning such as "knowledge base not found", skip the KB check, tell the user why, and continue the review. Keep the script's first line (KB path, version, hit count) for the step 6c summary.

**Severity precedence** (most specific wins): high-risk patterns doc → Security KB entry `severity_anchor` (mapping in the agent file) → driver guidelines → language skill → table in step 5. Exception: if this diff can concretely cause a crash, data corruption, or a security hole, rate it **High** and state the trigger.

## 3. Gather context (MANDATORY)

Launch `Explore` agents (in parallel, at "medium" breadth). Name the exact files and functions, and ask for the relevant code verbatim with `path:line`:

- **Code:** the full body of every changed function; the class/struct declaration in its header (member types, inheritance, locks, members); at least one direct caller if a signature changed; the RPC/IDL counterpart (`AsusGlideX` ↔ `AsusGlideXClient`) if RPC code changed.
- **Docs:** the source values the doc cites (RPC codes, enums, constants, type names), and whether each referenced path exists.

Many Medium and High issues can't be seen in the diff alone, so don't skip this step.

## 4. Structured analysis

Before writing any finding, go through **every** checklist category from the loaded files. Write one line per category in a collapsed internal pass, using "No issues found." when a category is clean. Don't skip categories. Add a **Security KB** category with one line per hit entry: `KB-xxx — Detection #N matches at path:line` or `KB-xxx — not applicable: <reason>`. Only report findings that are in the changed code, or in unchanged code the diff breaks (Scope Rules).

## 5. Severity

| Severity | Definition |
|---|---|
| **High** | Crash, data corruption, security vulnerability, or resource leak in production. |
| **Medium** | Incorrect behavior under specific conditions; missing error handling that fails silently; thread-safety issue causing intermittent bugs. |
| **Low** | Style inconsistency, minor readability, non-idiomatic but correct. |

Don't downgrade a severity to be polite. Severities from the loaded docs override this table.

## 6. Confirm findings with the user

If there are no findings, go to step 7.

**6a. One finding at a time.** For finding N:
1. In chat, write the detailed finding using the loaded skill's format (**Issue / Severity / Explanation / Suggestion**) with `path:line`.
2. Then call `AskUserQuestion` with one question. Header: `Finding N`. Question: `[Severity] <title> — <path:line>. Accept?` Options: `Accept` (Valid; include in the fix list) and `Dismiss` (Not an issue; give the reason via Other or in the next message).
3. Wait for the answer before showing the next finding.

**6b. Dismissals.**
- **Low:** accept the dismissal.
- **Medium/High:** take the user's reason and check it against the code (use `Explore` if needed). If the reason is weak or wrong, explain why the issue is real and ask about **only that finding** again. If the reason holds, accept it and record it as `(<concise reason>)` for the Review Summary block.

**6c. Summary.** Print:
```
**Review Summary**

Security KB: <path> @ <version> — N entries hit, M became findings

1. [High] [KB-PRV-004] <title> — Accepted
2. [Low] <title> — Dismissed
...
```
Then call `AskUserQuestion`. Header: `Summary`. Question: `N accepted, M dismissed. Confirm?` Options: `Confirm`, `Revise`. **Revise** goes back to 6a.

## 7. Next action

| Mode | Confirmed findings | No findings / all dismissed |
|---|---|---|
| Uncommitted | **Fix loop** | Report the review is complete. Don't amend or offer a cross-check. Tell the user to run `/code-review` after committing. |
| First-pass | **Fix loop** | Call `AskUserQuestion` (header `Passed`): `No Medium/High issues. Commit now or Codex cross-check?` Options: `Commit now`, `Codex cross-check`. |
| Second-pass | **Fix loop**; don't offer another cross-check | **Stamp** with `opus, codex` |

- **Commit now:** **Stamp** with `opus`.
- **Codex cross-check:** don't amend. Tell the user to open VS Code Copilot Chat, pick the **ReviewerCodex** agent, and send: `Cross-check the latest commit. This is a second-pass cross-check after Opus (Claude Code) found no Medium/High issues. Use git show HEAD to obtain the diff.` ReviewerCodex stamps `opus, codex` itself. End your turn.

### Fix loop (Planner → Executor)

1. **Plan.** For each confirmed finding, research with `Explore`, then write a concrete plan: file/function, the exact change, risks, and how to verify (the build command for that module from `PROJECT-MAP.md` §3). Keep it minimal: no refactors beyond what the finding needs.
2. Call `AskUserQuestion` (header `Fix plan`): `Apply this plan?` Options: `Apply`, `Revise`, `Stop here`. **Stop here** ends the turn, leaving the plan in chat.
3. **Apply** (requires the user's explicit Apply; Edit, Write and Bash prompts appear at this point):
   - Edit only what the plan says, and match the surrounding style. Don't add non-ASCII to C/C++ files.
   - Build if the module has a build command. If it fails, fix it before going on.
   - Print the deviation report: ✅ done as planned / ⚠️ deviated (what and why) / ❌ not done (why). If anything deviated significantly, confirm it with `AskUserQuestion`.
4. **Commit.**
   - Stage only the files you edited (`git add <files>`); never `git add -A` or `git add .`.
   - In commit modes, **amend** the reviewed commit. Call `AskUserQuestion` (header `Amend`) with options `<existing first line>` and `<suggested message>` (just one option if the existing message is already fine). Then amend.
   - In uncommitted mode, don't commit. Leave the changes in the working tree and tell the user.
5. **Re-review.** Run this skill again from step 1 in the same mode (first-pass for commits). The commit is stamped only when a later pass is clean.

### Stamp (amend with the review tag)

1. `git log -1 --format=%B`, to keep the original message.
2. First-pass **Commit now**: `AskUserQuestion` (header `Commit msg`): `Review passed (Opus). Pick a commit message:` with options `<original first line>` and `<suggested message>` (`PROJECT-MAP.md` §4 format). Second-pass uses the same question with `No issues confirmed by either Codex or Opus.`
3. Build the message in this order, with a blank line between parts:
   original message → `[copilot-reviewed]: <opus | opus, codex>` → `Review Summary:` block (only if a Medium/High dismissal was accepted; one line each; never Low) → `Change-Id:` line, if there is one (always last).
   ```
   [module][fix] some commit message

   [copilot-reviewed]: opus, codex

   Review Summary:
   - [Medium] Data race on m_blFlag — Dismissed (one-way flag, benign on target platform)

   Change-Id: I1234567890abcdef
   ```
4. Write the message to a file in the scratchpad directory and run `git commit --amend -F <file>`. Using a file keeps newlines and `Change-Id`. Then run `git log -1` to confirm.
5. Tell the user the review is complete and which tag was stamped.

## Rules

- Only review the latest commit (or the uncommitted diff), unless the user explicitly asks for more.
- Comment only on new or modified code.
- Never suggest rewriting existing CJK/Big5 comment lines.
- Never amend, stage, or edit without the matching user confirmation from step 6 or 7.
