#!/bin/bash

# Paths relative to ./tests
ISA_DIR="../install/share/riscv-tests/isa"
TEST_LIST_FILE="isa_tests_list.txt"

# Ensure directory and file exist
if [[ ! -d "$ISA_DIR" ]]; then
    echo "ERROR: ELF directory not found: $ISA_DIR"
    exit 1
fi

if [[ ! -f "$TEST_LIST_FILE" ]]; then
    echo "ERROR: Test list file not found: $TEST_LIST_FILE"
    exit 1
fi

# Function to detect real ELF files
is_elf() {
    file "$1" | grep -q "ELF"
}

# Gather all ELF file names in the directory
declare -A elf_set
while IFS= read -r filepath; do
    if is_elf "$filepath"; then
        filename="$(basename "$filepath" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
        elf_set["$filename"]=1
    fi
done < <(find "$ISA_DIR" -maxdepth 1 -type f | sort)

# Parse test list: accept lines starting with rv32/rv64, even if prefixed with 'x' or '#'
declare -a test_list=()
while IFS= read -r line; do
    line="$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
    [[ -z "$line" ]] && continue

    if [[ "$line" =~ ^x[[:space:]]*(rv(32|64)[^[:space:]]*) ]]; then
        testname="${BASH_REMATCH[1]}"
        test_list+=("$testname")
    elif [[ "$line" =~ ^#[[:space:]]*(rv(32|64)[^[:space:]]*) ]]; then
        testname="${BASH_REMATCH[1]}"
        test_list+=("$testname")
    elif [[ "$line" =~ ^(rv(32|64)[^[:space:]]*) ]]; then
        testname="${BASH_REMATCH[1]}"
        test_list+=("$testname")
    fi
done < "$TEST_LIST_FILE"

# Detect test names that do not have a corresponding ELF
declare -a missing_elf=()
declare -A testname_set
for testname in "${test_list[@]}"; do
    testname_set["$testname"]=1
    if [[ -z "${elf_set[$testname]}" ]]; then
        missing_elf+=("$testname")
    fi
done

# Detect ELF files that are not mentioned in the test list
declare -a unlisted_elf=()
for elf_file in "${!elf_set[@]}"; do
    if [[ -z "${testname_set[$elf_file]}" ]]; then
        unlisted_elf+=("$elf_file")
    fi
done

# Output results
echo "============================================"
echo "ELF files NOT listed in isa_tests_list.txt:"
if [[ ${#unlisted_elf[@]} -eq 0 ]]; then
    echo "  none"
else
    printf "  %s\n" "${unlisted_elf[@]}" | sort
fi

echo
echo "Test list entries MISSING ELF files:"
if [[ ${#missing_elf[@]} -eq 0 ]]; then
    echo "  none"
else
    printf "  %s\n" "${missing_elf[@]}" | sort
fi
echo "============================================"


##!/bin/bash
#
## Paths relative to ./tests
#ISA_DIR="../install/share/riscv-tests/isa"
#TEST_LIST_FILE="isa_tests_list.txt"
#
## Ensure directories and files exist
#if [[ ! -d "$ISA_DIR" ]]; then
#    echo "ERROR: ELF directory not found: $ISA_DIR"
#    exit 1
#fi
#
#if [[ ! -f "$TEST_LIST_FILE" ]]; then
#    echo "ERROR: Test list file not found: $TEST_LIST_FILE"
#    exit 1
#fi
#
## Function to detect ELF
#is_elf() {
#    file "$1" | grep -q "ELF"
#}
#
## Collect actual ELF filenames (basename only) that are true ELF files
#declare -A elf_set
#while IFS= read -r filepath; do
#    if is_elf "$filepath"; then
#        filename="$(basename "$filepath" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
#        elf_set["$filename"]=1
#    fi
#done < <(find "$ISA_DIR" -maxdepth 1 -type f | sort)
#
## Collect all test names mentioned in isa_tests_list.txt, regardless of enable/disable/comment
#declare -a test_list=()
#while IFS= read -r line; do
#    line="$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
#    [[ -z "$line" ]] && continue
#
#    if [[ "$line" =~ ^x[[:space:]]+(rv(32|64)[a-z]*-[ptv]+-[a-z0-9_]+) ]]; then
#        testname="${BASH_REMATCH[1]}"
#    elif [[ "$line" =~ ^#[[:space:]]*(rv(32|64)[a-z]*-[ptv]+-[a-z0-9_]+) ]]; then
#        testname="${BASH_REMATCH[1]}"
#    elif [[ "$line" =~ ^(rv(32|64)[a-z]*-[ptv]+-[a-z0-9_]+) ]]; then
#        testname="${BASH_REMATCH[1]}"
#    else
#        continue  # skip anything that doesn’t look like a test
#    fi
#
##    # Extract test name from: plain name, "x testname", or "# testname"
##    if [[ "$line" =~ ^x[[:space:]]+(.+) ]]; then
##        testname="${BASH_REMATCH[1]}"
##    elif [[ "$line" =~ ^#[[:space:]]*([a-zA-Z0-9_-]+) ]]; then
##        testname="${BASH_REMATCH[1]}"
##    else
##        testname="$line"
##    fi
#
#    test_list+=("$testname")
#done < "$TEST_LIST_FILE"
#
## Find missing ELF files for tests in list
#declare -a missing_elf=()
#declare -A testname_set
#for testname in "${test_list[@]}"; do
#    testname_set["$testname"]=1
#    if [[ -z "${elf_set[$testname]}" ]]; then
#        missing_elf+=("$testname")
#    fi
#done
#
## Find ELF files that are not in the isa_tests_list.txt
#declare -a unlisted_elf=()
#for elf_file in "${!elf_set[@]}"; do
#    if [[ -z "${testname_set[$elf_file]}" ]]; then
#        unlisted_elf+=("$elf_file")
#    fi
#done
#
## Output
#echo "============================================"
#echo "ELF files NOT listed in isa_tests_list.txt:"
#if [[ ${#unlisted_elf[@]} -eq 0 ]]; then
#    echo "  none"
#else
#    printf "  %s\n" "${unlisted_elf[@]}" | sort
#fi
#
#echo
#echo "Test list entries MISSING ELF files:"
#if [[ ${#missing_elf[@]} -eq 0 ]]; then
#    echo "  none"
#else
#    printf "  %s\n" "${missing_elf[@]}" | sort
#fi
#echo "============================================"
#
#
#
###!/bin/bash
##
### Paths relative to ./tests
##ISA_DIR="../install/share/riscv-tests/isa"
##TEST_LIST_FILE="isa_tests_list.txt"
##
### Ensure directories and files exist
##if [[ ! -d "$ISA_DIR" ]]; then
##    echo "ERROR: ELF directory not found: $ISA_DIR"
##    exit 1
##fi
##
##if [[ ! -f "$TEST_LIST_FILE" ]]; then
##    echo "ERROR: Test list file not found: $TEST_LIST_FILE"
##    exit 1
##fi
##
### Function to detect ELF
##is_elf() {
##    file "$1" | grep -q "ELF"
##}
##
### Collect all actual ELF base filenames
##declare -A elf_set
##while IFS= read -r filepath; do
##    if is_elf "$filepath"; then
##        filename="$(basename "$filepath" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
##        elf_set["$filename"]=1
##    fi
##done < <(find "$ISA_DIR" -maxdepth 1 -type f | sort)
##
### Read valid test names from isa_tests_list.txt
##test_list=()
##while IFS= read -r line; do
##    # Clean and skip blank, comment, or disabled lines
##    line="$(echo "$line" | sed 's/^[[:space:]]*//;s/[[:space:]]*$//')"
##    [[ -z "$line" || "$line" =~ ^# || "$line" =~ ^x[[:space:]] ]] && continue
##    test_list+=("$line")
##done < "$TEST_LIST_FILE"
##
### Detect test names with no corresponding ELF
##missing_elf=()
##for testname in "${test_list[@]}"; do
##    if [[ -z "${elf_set[$testname]}" ]]; then
##        missing_elf+=("$testname")
##    fi
##done
##
### Detect ELF files not mentioned in the test list
##declare -A testname_set
##for testname in "${test_list[@]}"; do
##    testname_set["$testname"]=1
##done
##
##unlisted_elf=()
##for elf_file in "${!elf_set[@]}"; do
##    if [[ -z "${testname_set[$elf_file]}" ]]; then
##        unlisted_elf+=("$elf_file")
##    fi
##done
##
### Output
##echo "============================================"
##echo "Found in riscv-tests but not isa_tests_list.txt:"
##if [[ ${#unlisted_elf[@]} -eq 0 ]]; then
##    echo "  none"
##else
##    printf "  %s\n" "${unlisted_elf[@]}" | sort
##fi
##
##echo
##
##echo "Found in isa_tests_list.txt but not riscv-tests"
##if [[ ${#missing_elf[@]} -eq 0 ]]; then
##    echo "  none"
##else
##    printf "  %s\n" "${missing_elf[@]}" | sort
##fi
##echo "============================================"
