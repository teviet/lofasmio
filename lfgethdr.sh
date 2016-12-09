#! /bin/bash
# Usage: lfgethdr.sh INFILE... [> OUTFILE]
# Collect headers from LoFASM filterbank files.

for f in "$@"
do
    if [[ $(file "$f") == *gzip* ]]
    then echo "$f"":"; gunzip -c "$f" | sed -e '/^[^%]/q'; echo ""
    elif [[ $(head "$f" -c 4) == *BX ]]
    then echo "$f"":"; sed -e '/^[^%]/q' "$f"; echo ""
    fi
done
