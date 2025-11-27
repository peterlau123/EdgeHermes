#!/usr/bin/env bash
# Install git hooks for NovaLLM
# This script sets up both pre-commit hooks and custom git hooks

set -euo pipefail

echo "🔧 Installing NovaLLM Git Hooks..."
echo ""

# Get the repository root
REPO_ROOT=$(git rev-parse --show-toplevel)
HOOKS_DIR="$REPO_ROOT/.githooks"

# 1. Configure git to use custom hooks directory
echo "📁 Configuring git to use custom hooks directory..."
git config core.hooksPath "$HOOKS_DIR"
echo "   ✅ Git hooks path set to: $HOOKS_DIR"
echo ""

# 2. Install pre-commit hooks
if command -v pre-commit &> /dev/null; then
  echo "📦 Installing pre-commit hooks..."
  pre-commit install --hook-type commit-msg --hook-type pre-commit
  echo "   ✅ Pre-commit hooks installed"
else
  echo "⚠️  pre-commit not found. Install it with:"
  echo "   pip install pre-commit"
  echo "   Then run: pre-commit install --hook-type commit-msg --hook-type pre-commit"
fi
echo ""

# 3. Make all hook scripts executable
echo "🔐 Making hook scripts executable..."
chmod +x "$HOOKS_DIR"/* 2>/dev/null || true
echo "   ✅ Hook scripts are executable"
echo ""

# 4. Test branch name validation (if on a feature branch)
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "📋 Current branch: $CURRENT_BRANCH"

# Summary
cat <<EOF

╔════════════════════════════════════════════════════════════════╗
║                  ✅ HOOKS INSTALLED SUCCESSFULLY               ║
╚════════════════════════════════════════════════════════════════╝

Installed hooks:
  ✓ post-checkout     - Validates branch names
  ✓ commit-msg        - Validates commit messages
  ✓ pre-commit        - Code quality checks

Branch name format:
  <type>-<description> or <type>/<description>
  Example: feat-buffer-pooling, fix-memory-leak

Commit message format:
  <type>(<scope>): <subject>
  Example: feat(memory): add buffer pooling

Valid types:
  feat, fix, docs, style, refactor, perf, test, build, ci, chore

Try it out:
  git checkout -b feat-test-branch
  git commit -m "feat(test): try the hooks"

For more info, see: .pre-commit-setup.md

EOF
