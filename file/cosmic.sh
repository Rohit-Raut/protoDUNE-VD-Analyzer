#!/bin/bash
# Define input and output files
INPUT_FILE="run39350.txt"
OUTPUT_FILE="cosmic_run39350.txt"
ERROR_LOG="missing_files.log"

# Clear previous outputs
> "$OUTPUT_FILE"
> "$ERROR_LOG"

# Count total lines
TOTAL=$(wc -l < "$INPUT_FILE")
CURRENT=0

echo "Starting Rucio lookup for $TOTAL files..."

while read -r line || [[ -n "$line" ]]; do
    # Skip empty lines
    [[ -z "$line" ]] && continue
    ((CURRENT++))
    
    # Clean input line
    CLEAN_LINE=$(echo "$line" | tr -d '\r')
    
    # Get ALL PFNs
    ALL_PFNS=$(rucio replica list file --pfns "$CLEAN_LINE" 2>/dev/null)
    
    # 1. Try to find a Fermilab copy first (fnal.gov) - take ONLY the first match to avoid duplicates
    CHOSEN_PFN=$(echo "$ALL_PFNS" | grep "fnal.gov" | head -n 1)
    
    # 2. If no Fermilab copy, take ANY valid root copy (e.g. CERN, Nikhef) - again, only first to avoid duplicates
    if [[ -z "$CHOSEN_PFN" ]]; then
        CHOSEN_PFN=$(echo "$ALL_PFNS" | grep "root://" | head -n 1)
    fi
    
    if [[ -n "$CHOSEN_PFN" ]]; then
        echo "$CHOSEN_PFN" >> "$OUTPUT_FILE"
        printf "\rProgress: [%d/%d] Success" "$CURRENT" "$TOTAL"
    else
        echo "$CLEAN_LINE" >> "$ERROR_LOG"
        printf "\rProgress: [%d/%d] FAILED (Logged to $ERROR_LOG)" "$CURRENT" "$TOTAL"
    fi
done < "$INPUT_FILE"

echo -e "\n\nDone!"
echo "Valid PFNs: $(wc -l < "$OUTPUT_FILE")"
echo "Failed/Missing: $(wc -l < "$ERROR_LOG")"
