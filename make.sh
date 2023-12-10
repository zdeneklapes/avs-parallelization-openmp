#!/bin/bash

###########################################################
###########################################################
RM="rm -rfd"
RED='\033[0;31m'
NC='\033[0m'
GREEN='\033[0;32m'
CURRENT_DIR_NAME="avs-parallelization-openmp"

function clean() {
    # Folders
    # ENVIRONMENT VARIABLES:
    #   DEBUG: 1/0 if set to 1, only print commands, do not execute them

    # Check ENVIRONMENT VARIABLES
    if [ -z "$DEBUG" ]; then DEBUG=0; fi

    for folder in \
        "cmake-build-debug*" \
        "build_advisor" \
        "build"; do
        if [ "$DEBUG" -eq 1 ]; then find . -type d -iname "${folder}"; else find . -type d -iname "${folder}" | xargs ${RM} -rf; fi
    done

    # Files
    for file in \
        "*.DS_Store" \
        "tags" \
        "*.zip"; do
        if [ "$DEBUG" -eq 1 ]; then find . -type f -iname "${file}"; else find . -type f -iname "${file}" | xargs ${RM}; fi
    done
}

function build_local() {
    # Build project using cmake
    mkdir -p build
    cmake \
        -DCC="$(which gcc)" \
        -DCXX="$(which g++)" \
        -DLDFLAGS="-L/usr/local/opt/llvm/lib" \
        -DCPPFLAGS="-I/usr/local/opt/llvm/include" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_COMPILER_ID="GNU" \
        -DCMAKE_CXX_FLAGS="-O0 -g" \
        -Bbuild \
        -S.
    make -j -C build
}

function build_local_O3() {
    # Build project using cmake
    mkdir -p build
    cmake \
        -DCC="$(which gcc)" \
        -DCXX="$(which g++)" \
        -DLDFLAGS="-L/usr/local/opt/llvm/lib" \
        -DCPPFLAGS="-I/usr/local/opt/llvm/include" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_COMPILER_ID="GNU" \
        -DCMAKE_CXX_FLAGS="-O3" \
        -Bbuild \
        -S.
    make -j -C build
}

function build_barbora() {
    # Build project using cmake
    mkdir -p build
    cmake \
        -Bbuild \
        -S.
    make -j -C build
}

function pack() {
    # Clean and Zip project
    # ENVIRONMENT VARIABLES:
    #   DEBUG: 1/0 if set to 1, only print commands, do not execute them

    # Check ENVIRONMENT VARIABLES
    if [ -z "$DEBUG" ]; then DEBUG=0; fi

    ZIP_NAME="xlapes02.zip"

    CMD="zip -r '$ZIP_NAME' \
        PMC-xlapes02.txt \
        parallel_builder/*.cpp \
        parallel_builder/*.h \
        *.png"

    # Copy png files from the newest folder inside tmp/backups
    CMD2="cp \$(ls -td tmp/backups/*/ | head -n 1)build_evaluate/*.png ."

    if [ $DEBUG -eq 1 ]; then echo "$CMD"; else eval "$CMD2"; fi
    if [ $DEBUG -eq 1 ]; then echo "$CMD"; else eval "$CMD"; fi
}

function test_fast() {
    # run clean
    DEBUG=0 clean
    DEBUG=0 build_local
    make -j -C build_local
    for i in "ref" "line" "batch"; do
        echo "Running test $i"
        ./build/mandelbrot -c ref -s 4096 res.npz
    done
    ./build/mandelbrot -c ref -s 4096 res.npz
}

function killall_barbora() {
    ssh zdeneklapes@login1.barbora.it4i.cz -i ~/.ssh/id_rsa_avs 'killall -u $USER'
    ssh zdeneklapes@login2.barbora.it4i.cz -i ~/.ssh/id_rsa_avs 'killall -u $USER'
}

function slurm_sinfo() {
    # Print info about nodes
    sinfo --long
    sinfo --summarize
}

function slurm_squeue() {
    # Print info about jobs
    squeue --me -l
}

function slurm_watch_squeue() {
    # Watch squeue
    watch -n 1 "squeue -l | awk 'BEGIN{sum=0; psum = 0} {if(\$5 == \"RUNNING\") sum +=\$8; if (\$5 == \"PENDING\" && \$2 == \"qcpu_exp\") psum +=1;} END {printf \"%+4s/201 nodes used at the moment\n\", sum; printf \"%+4s nodes free\n\", 201-sum; printf \"%+4s pending jobs in queue qcpu_exp\n\", psum}'"
    #    watch -n 1 "squeue --me -l"
}

function slurm_cancel_all() {
    # Cancel all jobs
    scancel --user=$USER
}

function slurm_sbatch() {
    # Submit jobs to slurm: vtune.sl and evaluate.sl
    # ENVIRONMENT VARIABLES:
    #   DEBUG: 1/0 if set to 1, only print commands, do not execute them

    # Check ENVIRONMENT VARIABLES
    if [ -z "$DEBUG" ]; then DEBUG=0; fi

    echo "Submitting jobs to slurm"
    file1="$(pwd)/${CURRENT_DIR_NAME}/vtune.sl"
    file2="$(pwd)/${CURRENT_DIR_NAME}/evaluate.sl"

    for file in ${file1} ${file2}; do
        cd "$(dirname ${file})"
        if [ $DEBUG -eq 1 ]; then echo "sbatch ${file}"; else sbatch ${file1}; fi
        cd -
    done
}

function help() {
    # Print usage on stdout
    echo "Available functions:"
    for file in ${BASH_SOURCE[0]}; do
        function_names=$(cat ${file} | grep -E "(\ *)function\ +.*\(\)\ *\{" | sed -E "s/\ *function\ +//" | sed -E "s/\ *\(\)\ *\{\ *//")
        for func_name in ${function_names[@]}; do
            printf "    $func_name\n"
            awk "/function ${func_name}()/ { flag = 1 }; flag && /^\ +#/ { print \"        \" \$0 }; flag && !/^\ +#/ && !/function ${func_name}()/  { print "\n"; exit }" ${file}
        done
    done
}

function send_code_to_barbora() {
    # This function is used to send code to Barbora

    # Create archive
    zip_name="xlapes02.zip"

    # Create archive
    #    git archive -o ${zip_name} HEAD
    files=$(git ls-files)

    # remove all pdf files from files var
    files=$(echo ${files} | sed -E "s/\.pdf//g")

    # zip files
    zip -r ${zip_name} ${files}

    # Send archive
    scp ${zip_name} 'avs_barbora:~/repos'

    # rm archive on local machine and on server
    rm ${zip_name}
    #    ssh avs_barbora "cd ~/repos && rm -rfd ${CURRENT_DIR_NAME} && unzip -d ${CURRENT_DIR_NAME} ${zip_name}"
    ssh avs_barbora "cd ~/repos && rm -rfd ${CURRENT_DIR_NAME} && unzip -d ${CURRENT_DIR_NAME} ${zip_name} && rm ${zip_name}"
}

function test_on_barbora() {
    # This function is used to test the code on Barbora
    # This copy code to Barbora, build it and run sbatch advisor.sl and evaluate.sl
    send_code_to_barbora
    # Run advisor and evaluate
    ssh avs_barbora "cd ~/repos/${CURRENT_DIR_NAME} && sbatch evaluate.sl; sbatch vtune.sl"
}

function backup() {
    # This function is used to backup the code on Barbora

    CMD="squeue --me -l -t RUNNING --noheader | wc -l"
    CMD2="squeue --me -l -t PENDING --noheader | wc -l"
    while [ 1 ]; do
        running_jobs=$(ssh avs_barbora "$CMD")
        pending_jobs=$(ssh avs_barbora "$CMD2")
        if [ "$running_jobs" -eq 0 ] && [ "$pending_jobs" -eq 0 ]; then
            echo -e "${GREEN}All jobs finished${NC}"
            time=$(date +%Y-%m-%d_%H-%M)
            for i in "build_evaluate" "build_vtune" "build"; do
                mkdir -p tmp/backups/${time}/${i}
            done
            #            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build_evaluate/tmp_*" tmp/backups/${time}/build_evaluate/
            #            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build_evaluate/*.optrpt" tmp/backups/${time}/build_evaluate/
            #            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build_evaluate/CMakeFiles/PMC.dir/*"
            #
            #            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build_vtune/vtune-*" tmp/backups/${time}/build_vtune/
            #            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build_vtune/CMakeFiles/PMC.dir/*" tmp/backups/${time}/build_vtune/

            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build_evaluate/*" tmp/backups/${time}/build_evaluate/
            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build_vtune/*" tmp/backups/${time}/build_vtune/
            rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/build/*" tmp/backups/${time}/build/

            for i in "csv" "out"; do
                rsync -avz -e ssh "avs_barbora:\$(pwd)/repos/${CURRENT_DIR_NAME}/*.${i}" tmp/backups/${time}/
            done
            break
        else
            echo -e "${RED}Waiting (5 seconds) for jobs to finish${NC} - RUNNING: $running_jobs, PENDING: $pending_jobs"
            sleep 5
        fi
    done
}

function watch_queue() {
    # Watch queue
    watch -n 1 "squeue -l | awk 'BEGIN{sum=0; psum = 0} {if(\$5 == \"RUNNING\") sum +=\$8; if (\$5 == \"PENDING\" && \$2 == \"qcpu_exp\") psum +=1;} END {printf \"%+4s/201 nodes used at the moment\n\", sum; printf \"%+4s nodes free\n\", 201-sum; printf \"%+4s pending jobs in queue qcpu_exp\n\", psum}'"
}

function usage() {
    # Print usage on stdout
    help
}

function check_mem_leaks() {
    # Check memory leaks using valgrind
    # ENVIRONMENT VARIABLES:
    #   DEBUG: 1/0 if set to 1, only print commands, do not execute them

    # Check ENVIRONMENT VARIABLES
    if [ -z "$DEBUG" ]; then DEBUG=0; fi

    build_local
    #    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=tmp/valgrind.out ./build/mandelbrot -c batch -s 512 tmp/res_batch.npz
    valgrind ./build/PMC --level 0.15 --grid 8 --builder loop --threads 1 data/bun_zipper_res1.pts loop.obj
}

function test() {
    # Run PMC with different parameters
    # ENVIRONMENT VARIABLES:
    #   DEBUG: 1/0 if set to 1, only print commands, do not execute them
    #   REF: 1/0 if set to 1, run reference implementation
    #   LOOP: 1/0 if set to 1, run loop implementation
    #   TREE: 1/0 if set to 1, run tree implementation
    #   THREADS: number of threads to use
    #   GRID: grid size


    if [ -z "$DEBUG" ]; then DEBUG=0; fi
    if [ -z "$REF" ]; then REF=0; fi
    if [ -z "$LOOP" ]; then LOOP=0; fi
    if [ -z "$TREE" ]; then TREE=0; fi
    if [ -z "$THREADS" ]; then THREADS=0; fi
    if [ -z "$GRID" ]; then GRID=32; fi

    if [ $REF -eq 1 ]; then
        echo "Running ref"
        ./build/PMC --level 0.15 --grid ${GRID} --builder ref --threads ${THREADS} data/bun_zipper_res1.pts tmp/ref.obj
    fi

    if [ $LOOP -eq 1 ]; then
        echo "Running loop"
        ./build/PMC --level 0.15 --grid ${GRID} --builder loop --threads ${THREADS} data/bun_zipper_res1.pts tmp/loop.obj
    fi

    if [ $TREE -eq 1 ]; then
        echo "Running tree"
        ./build/PMC --level 0.15 --grid ${GRID} --builder tree --threads ${THREADS} data/bun_zipper_res1.pts tmp/tree.obj
    fi

    if [ $LOOP -eq 1 ]; then
        echo "Running check ref vs tree"
        python3 ./scripts/check_output.py tmp/ref.obj tmp/loop.obj
    fi

    if [ $TREE -eq 1 ]; then
        echo "Running check ref vs loop"
        python3 ./scripts/check_output.py tmp/ref.obj tmp/tree.obj
    fi

    if [ $LOOP -eq 1 ] && [ $TREE -eq 1 ]; then
        echo "Running check loop vs tree"
        python3 ./scripts/check_output.py tmp/loop.obj tmp/tree.obj
    fi
}

function die() {
    # Print error message on stdout and exit
    printf "${RED}ERROR: $1${NC}\n"
    help
    exit 1
}

function main() {
    # Main function: Call other functions based on input arguments
    [[ "$#" -eq 0 ]] && die "No arguments provided"
    while [ "$#" -gt 0 ]; do
        "$1" || die "Unknown function: $1()"
        shift
    done
}

main "$@"
