#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

declare -xri EXIT_FAILURE=1
declare -xri EXIT_SUCCESS=0
declare -xi exit_code=$EXIT_FAILURE

function check_dependency() {
    declare -xr dependency_command="$1"
    if type "$(command -v "$dependency_command")" > /dev/null 2>& 1; then
        return $EXIT_SUCCESS
    else
        >& 2 echo 'Dependency `'"$dependency_command"'` not found'
        return $EXIT_FAILURE
    fi
}

check_dependency basename
check_dependency dirname
check_dependency realpath

if check_dependency tput; then
    declare -xr red="$(tput setaf 1)"
    declare -xr green="$(tput setaf 2)"
    declare -xr reset="$(tput sgr0)"
else
    declare -xr red=''
    declare -xr green=''
    declare -xr reset=''
fi

function test_solver() {
    declare -xr filename="$1"
    declare -xr expected="$(basename "$filename" .png)"
    declare -xr solved="$("$SOLVER" "$filename")"
    if [ "$solved" = "$expected" ]; then
        echo "[ ${green}PASS$reset ] $expected.png"
        return $EXIT_SUCCESS
    else
        echo "[ ${red}FAIL$reset ] $expected.png"
        return $EXIT_FAILURE
    fi
}

function iterate_tests() {
    declare -xi failed_test_count=0
    for file in "$__DIR__"/images/*.png; do
        test_solver "$file" || (( ++failed_test_count ))
    done
    echo -------------------
    if [ "$failed_test_count" = 0 ]; then
        echo "$(basename "$__FILE__"): ${green}All tests completed successfully$reset"
        return $EXIT_SUCCESS
    else
        echo "$(basename "$__FILE__"): $red$failed_test_count test(s) failed$reset"
        return $EXIT_FAILURE
    fi
}

declare -xr __FILE__="$(realpath "${BASH_SOURCE[0]}")"
declare -xr __DIR__="$(dirname "$__FILE__")"
declare -xr SOLVER="$__DIR__/../solver"

if [ -f "$SOLVER" ] && [ -x "$SOLVER" ]; then
    iterate_tests && exit_code=$EXIT_SUCCESS
else
    >& 2 echo 'solver executable not found; run `make` first'
fi

exit $exit_code
