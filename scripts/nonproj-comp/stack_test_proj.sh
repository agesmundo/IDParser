#!/bin/bash 

if [ -z "$1" ]; then
echo "Usage: ./tack_test_proj.sh <project_folder> [language_ID]"
exit 
fi

echo "cd $1"
	cd $1
PROJECT_FOLDER=$(pwd)

	DATA_FOLDER=$PROJECT_FOLDER/data/bkp
	SCRIPT_FOLDER=$PROJECT_FOLDER/scripts
	SCRIPT_FOLDER_2009=$SCRIPT_FOLDER/conll2009
	SCRIPT_FOLDER_PROJ=$SCRIPT_FOLDER/nonproj-comp

	LANGUAGE_FOLDERS[0]="${DATA_FOLDER}/Ca09"
	LANGUAGE_FOLDERS[1]="${DATA_FOLDER}/Ch09"
	LANGUAGE_FOLDERS[2]="${DATA_FOLDER}/Cz09"
	LANGUAGE_FOLDERS[3]="${DATA_FOLDER}/En09"
	LANGUAGE_FOLDERS[4]="${DATA_FOLDER}/Ge09"
	LANGUAGE_FOLDERS[5]="${DATA_FOLDER}/Ja09"
	LANGUAGE_FOLDERS[6]="${DATA_FOLDER}/Sp09"

	TRAIN_FILES[0]="${LANGUAGE_FOLDERS[0]}/CoNLL2009-ST-Catalan-train"
	TRAIN_FILES[1]="${LANGUAGE_FOLDERS[1]}/CoNLL2009-ST-Chinese-train"
	TRAIN_FILES[2]="${LANGUAGE_FOLDERS[2]}/CoNLL2009-ST-Czech-train"
	TRAIN_FILES[3]="${LANGUAGE_FOLDERS[3]}/CoNLL2009-ST-English-train"
	TRAIN_FILES[4]="${LANGUAGE_FOLDERS[4]}/CoNLL2009-ST-German-train"
	TRAIN_FILES[5]="${LANGUAGE_FOLDERS[5]}/CoNLL2009-ST-Japanese-train"
	TRAIN_FILES[6]="${LANGUAGE_FOLDERS[6]}/CoNLL2009-ST-Spanish-train"

	OUT_FILES[0]="$SCRIPT_FOLDER_PROJ/Catalan"
	OUT_FILES[1]="$SCRIPT_FOLDER_PROJ/Chinese"
	OUT_FILES[2]="$SCRIPT_FOLDER_PROJ/Czech"
	OUT_FILES[3]="$SCRIPT_FOLDER_PROJ/English"
	OUT_FILES[4]="$SCRIPT_FOLDER_PROJ/German"
	OUT_FILES[5]="$SCRIPT_FOLDER_PROJ/Japanese"
	OUT_FILES[6]="$SCRIPT_FOLDER_PROJ/Spanish"


	C08_EXT=.c08
	RES_EXT=-proj-metrics.txt

	FROM_ID=0;
	TO_ID=${#TRAIN_FILES[@]}

	if [ -n "$2" ]; then
		FROM_ID=$2
		TO_ID=$2
		let FROM_ID=FROM_ID-1
	fi

	COUNTER=$FROM_ID
	while [ $COUNTER -lt  $TO_ID ]; do
		
		echo "$SCRIPT_FOLDER_2009/conll09_to_conll08.pl ${TRAIN_FILES[$COUNTER]} ${OUT_FILES[$COUNTER]}$C08_EXT"
		$SCRIPT_FOLDER_2009/conll09_to_conll08.pl ${TRAIN_FILES[$COUNTER]} ${OUT_FILES[$COUNTER]}$C08_EXT

		echo "" > ${OUT_FILES[$COUNTER]}$RES_EXT
		echo "Syntax and Semantics:" >> ${OUT_FILES[$COUNTER]}$RES_EXT
		echo "cat ${OUT_FILES[$COUNTER]}$C08_EXT | $SCRIPT_FOLDER_PROJ/stack_test_proj_tot > $SCRIPT_FOLDER_PROJ/stack_test_proj_tot_res"
		cat ${OUT_FILES[$COUNTER]}$C08_EXT | $SCRIPT_FOLDER_PROJ/stack_test_proj_tot > $SCRIPT_FOLDER_PROJ/stack_test_proj_tot_res
		tail -21 $SCRIPT_FOLDER_PROJ/stack_test_proj_tot_res >> ${OUT_FILES[$COUNTER]}$RES_EXT

		echo "" >> ${OUT_FILES[$COUNTER]}$RES_EXT
		echo "Semantics:" >> ${OUT_FILES[$COUNTER]}$RES_EXT
		echo "cat ${OUT_FILES[$COUNTER]}$C08_EXT | $SCRIPT_FOLDER_PROJ/stack_test_proj_sem > $SCRIPT_FOLDER_PROJ/stack_test_proj_sem_res"
		cat ${OUT_FILES[$COUNTER]}$C08_EXT | $SCRIPT_FOLDER_PROJ/stack_test_proj_sem > $SCRIPT_FOLDER_PROJ/stack_test_proj_sem_res
		tail -15 $SCRIPT_FOLDER_PROJ/stack_test_proj_sem_res >> ${OUT_FILES[$COUNTER]}$RES_EXT

		echo "" >> ${OUT_FILES[$COUNTER]}$RES_EXT
		echo "Syntax:" >> ${OUT_FILES[$COUNTER]}$RES_EXT
		echo "cat ${OUT_FILES[$COUNTER]}$C08_EXT | $SCRIPT_FOLDER_PROJ/stack_test_proj_syn > $SCRIPT_FOLDER_PROJ/stack_test_proj_syn_res"
		cat ${OUT_FILES[$COUNTER]}$C08_EXT | $SCRIPT_FOLDER_PROJ/stack_test_proj_syn > $SCRIPT_FOLDER_PROJ/stack_test_proj_syn_res
		tail -15 $SCRIPT_FOLDER_PROJ/stack_test_proj_syn_res >> ${OUT_FILES[$COUNTER]}$RES_EXT

		rm ${OUT_FILES[$COUNTER]}$C08_EXT
		rm $SCRIPT_FOLDER_PROJ/stack_test_proj_tot_res
		rm $SCRIPT_FOLDER_PROJ/stack_test_proj_sem_res
		rm $SCRIPT_FOLDER_PROJ/stack_test_proj_syn_res

		echo ""
		echo "OUT: Results saved in: ${OUT_FILES[$COUNTER]}$RES_EXT"
		echo ""

		let COUNTER=COUNTER+1 
	done
