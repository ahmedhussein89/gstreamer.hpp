#!/usr/bin/env bash
# scripts/check-concepts.sh
#
# CI gate: fails if any template in include/**/*.hpp has an unconstrained
# type parameter (bare 'typename T' / 'class T').
#
# A parameter is considered constrained when any of the following holds:
#   1. The template head uses a concept name:  template <ConceptName T>
#   2. The declaration or its next two lines contain a 'requires' clause.
#   3. The template head declares a concept:   template <typename T> concept
#
# Run from the repo root: bash scripts/check-concepts.sh
# Exit 0 = all clear; exit 1 = violations found.

set -euo pipefail
REPO_ROOT="$(git -C "$(dirname "$0")" rev-parse --show-toplevel)"
cd "$REPO_ROOT"

mapfile -d '' HPP_FILES < <(find include -name '*.hpp' -print0 | sort -z)

violations=()

for f in "${HPP_FILES[@]}"; do
  mapfile -t lines < "$f"
  n=${#lines[@]}
  for (( i=0; i<n; i++ )); do
    line="${lines[$i]}"

    # Skip lines that don't contain a template declaration.
    [[ "$line" =~ template[[:space:]]*\< ]] || continue

    # Skip concept definitions — they must use bare typename by language rule.
    # The 'concept' keyword may appear on the same line or on the very next line.
    [[ "$line" =~ concept[[:space:]] ]] && continue

    # Skip using-alias declarations.
    [[ "$line" =~ ^[[:space:]]*using[[:space:]] ]] && continue

    # Extract the angle-bracket argument list (single-line only).
    if [[ "$line" =~ template[[:space:]]*\<([^\>]*)\> ]]; then
      params="${BASH_REMATCH[1]}"
    else
      params="$line"
    fi

    # If the params contain a bare typename/class keyword followed by an
    # identifier (with optional variadic '...' in between), it is potentially
    # unconstrained.
    if [[ "$params" =~ (typename|class)([[:space:]]+|[[:space:]]*\.\.\.[[:space:]]*)[A-Za-z_] ]]; then
      # Build a 3-line context window (current + next 2) to detect requires.
      ctx="$line"
      (( i+1 < n )) && ctx+=" ${lines[$((i+1))]}"
      (( i+2 < n )) && ctx+=" ${lines[$((i+2))]}"

      # Allow concept definitions (concept keyword on current or next line).
      [[ "$ctx" =~ concept[[:space:]] ]] && continue
      # Allow if a requires clause is present in the context window.
      [[ "$ctx" =~ requires ]] && continue

      violations+=("$f:$((i+1)): $line")
    fi
  done
done

if (( ${#violations[@]} > 0 )); then
  echo "ERROR: Unconstrained template type parameters found in include/:"
  printf '  %s\n' "${violations[@]}"
  echo ""
  echo "Every 'typename T' / 'class T' template parameter must use a"
  echo "named concept (e.g. 'template <FlagEnum Bits>') or a requires clause."
  echo "See docs/concepts-roadmap.md for the vocabulary and phase plan."
  exit 1
fi

echo "OK: All template type parameters in include/ are constrained."
