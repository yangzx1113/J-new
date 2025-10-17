#!/bin/bash

# 定义timeout和执行程序
out=60

solver="timeout -s 15 $out ./NuWLS-c_pre"

# 处理函数，传入文件路径作为参数
process_file() {
    file="$1"
    result_file="$2"
    filename=$(basename "$file")  # 获取文件名
    dirname=$(dirname "$file")    # 获取文件所在目录名

    echo "处理文件: $filename" 
    
    # 执行 hywalk 程序，并将输出保存到以文件名命名的txt文件中
    timeout -s 15 500 ./J_new "$filename"

    start_time=$(date +%s.%N)  # 记录开始时间
    
    $solver "$filename" | while IFS= read -r line; do
        elapsed=$(printf "%.3f" "$(echo "$(date +%s.%N) - $start_time" | bc)")
        echo "[$elapsed] $line"
    done > "${filename%.*}.txt"
    
    # 移动结果文件到相应的结果文件夹中
    mv "${filename%.*}.txt" "$result_file"

    # 删除已处理的文件
    rm "$filename"
}

# 解压函数，传入文件路径作为参数
extract_file() {
    file="$1"
    resultfile="$2"
    filename=$(basename "$file")  # 获取文件名

    echo "解压文件: $filename"

    # 根据文件扩展名进行解压
    case "$filename" in
        *.gz)
            # 解压gz文件
            newfile=${filename%.gz}
            gunzip -c "$file" > "$newfile"
            
            ;;
        *.xz)
            # 解压xz文件
            xz -d -k "$file"
            newfile=${filename%.xz}
            ;;
        *.wcnf)
            # 复制wcnf文件到当前目录
            cp "$file" .
            newfile=${filename}
            ;;
        *)
            echo "未知文件格式: $filename"
            ;;
    esac
    #执行程序
    process_file "$newfile" "$resultfile"
}

# 遍历处理文件夹
for folder in "2020uw"; do
    echo "处理文件夹: $folder"
    count=0
    
    result="result_$folder"
    
    mkdir -p "$result"

    # 解压文件
    for file in ../../$folder/*; do
        extract_file "$file" "$result"
        ((count++))
        echo "处理个数: $count"
    done
done

echo "处理完成"