#!/bin/bash

OUTDIR="cheader"
DISTDIR="dist"
STATIC_INCLUDES_DIR="static_includes"
INCLUDE_DIR="../wdoc"

if [ ! -d $OUTDIR ]; then
    echo "$OUTDIR not found!"
    mkdir "$OUTDIR"
fi
if [ "$(ls -A $OUTDIR)" ]; then
    echo "Removing all files from $OUTDIR"
    rm $OUTDIR/*.h || true
else
    echo "$OUTDIR is empty, nothing needs to be removed"
fi

htmlfiles=(index_s.htm.gz control_s.htm.gz config.htm.gz setup.htm.gz logging.htm.gz gravity.htm.gz gravity_e32.htm.gz pressure.htm.gz backup.htm.gz)

variables=(data_index_htm_gz control_htm_gz config_htm_gz setup_htm_gz logging_htm_gz gravity_htm_gz gravity_htm_gz pressure_htm_gz backup_htm_gz)

outfiles=(index_htm control_htm config_htm setup_htm log_htm gdc_htm gdc_e32_htm pressure_htm backup_htm)

languages=(norwegian english spanish portuguese-br slovak chinese italian)


gen_C_file()
{
lang=$1;
for ((index=0; index<${#htmlfiles[@]}; index++)); do
    srcdir="dist/$lang"

    echo "[$index]: ${htmlfiles[$index]}"
    input="$srcdir/${htmlfiles[$index]}"
    output="$OUTDIR/${lang}_${outfiles[$index]}.h"
    variable=${variables[$index]}
    echo "input: $input output file: $output with variables $variable "
    xxd -i  "$input" > $output
    echo "processing $output"
    sed -i "s/unsigned char .\+\[\]/const unsigned char $variable\[\] PROGMEM/" $output
done
}

for lang in "${languages[@]}"; do
    echo -e "\n*** Processing $lang"
    gen_C_file $lang
done

echo -e "\n*** Exporting files from OUTDIR [${OUTDIR}] to the INCLUDE_DIR [${INCLUDE_DIR}]"
if [ ! -d $INCLUDE_DIR ]; then
    echo "- $INCLUDE_DIR not found! Now creating ..."
    mkdir "$INCLUDE_DIR"
fi
if [ "$(ls -A $INCLUDE_DIR)" ]; then
    echo "- Files detected in [$INCLUDE_DIR]! -> Removing all files from [$INCLUDE_DIR]"
    rm $INCLUDE_DIR/*.h || true
fi
echo "- Start copying everything from [$OUTDIR] to [$INCLUDE_DIR]"
cp $OUTDIR/*.h $INCLUDE_DIR
echo "- Injecting static includes"
cp $STATIC_INCLUDES_DIR/*.h $INCLUDE_DIR
echo -e "\n*** DONE ***"