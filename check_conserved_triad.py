import os
import subprocess
from Bio import SeqIO
from collections import Counter
from pathlib import Path

# ================== 用户设置区域 ==================
KNOWN_FASTA = "9can.fasta"
UNKNOWN_FASTA = "END.fasta"
OUTPUT_DIR = Path("aligned_results")
TRIAD_DIR = Path("triad_analysis")
SUMMARY_FILE = TRIAD_DIR / "triad_summary.tsv"
TEMP_INPUT = "temp_input.fasta"
TEMP_OUTPUT = "temp_output.fasta"

# 创建输出目录
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(TRIAD_DIR, exist_ok=True)

# ================== 工具函数 ==================

def read_fasta(file_path):
    """读取FASTA文件，返回一个字典 {id: sequence}"""
    sequences = {}
    for record in SeqIO.parse(file_path, "fasta"):
        sequences[record.id] = str(record.seq)
    return sequences

def write_fasta(sequences, file_path):
    """将序列字典写入FASTA文件"""
    with open(file_path, "w") as f:
        for seq_id, seq in sequences.items():
            f.write(f">{seq_id}\n{seq}\n")

def run_mafft(input_file, output_file):
    """调用 MAFFT 进行多序列比对"""
    cmd = ["mafft", "--auto", "--quiet", input_file]
    with open(output_file, "w") as f:
        subprocess.run(cmd, stdout=f, check=True)

def find_conserved_triad(sequences):
    """查找常见的 Ser-His-Asp 三联体位置"""
    positions = {'S': [], 'H': [], 'D': []}
    for seq in sequences.values():
        for i, aa in enumerate(seq):
            if aa == 'S':
                positions['S'].append(i)
            elif aa == 'H':
                positions['H'].append(i)
            elif aa == 'D':
                positions['D'].append(i)

    ser_pos = Counter(positions['S']).most_common(1)[0][0] if positions['S'] else -1
    his_pos = Counter(positions['H']).most_common(1)[0][0] if positions['H'] else -1
    asp_pos = Counter(positions['D']).most_common(1)[0][0] if positions['D'] else -1

    return ser_pos, his_pos, asp_pos

def check_unknown_triad(aligned_file, unknown_id):
    """检查未知蛋白是否具有保守三联体"""
    aligned_seqs = read_fasta(aligned_file)
    try:
        ser_pos, his_pos, asp_pos = find_conserved_triad(aligned_seqs)
        unknown_seq = aligned_seqs.get(unknown_id, "")
        if not unknown_seq:
            return False, -1, -1, -1
        s = unknown_seq[ser_pos] if ser_pos >= 0 and ser_pos < len(unknown_seq) else ''
        h = unknown_seq[his_pos] if his_pos >= 0 and his_pos < len(unknown_seq) else ''
        d = unknown_seq[asp_pos] if asp_pos >= 0 and asp_pos < len(unknown_seq) else ''
        return (s == 'S' and h == 'H' and d == 'D'), ser_pos, his_pos, asp_pos
    except Exception as e:
        print(f"分析错误：{e}")
        return False, -1, -1, -1

# ================== 主程序逻辑 ==================

print("✅ 开始处理...")

# 写入摘要文件头
with open(SUMMARY_FILE, "w") as f:
    f.write("Protein_ID\tSer_Position\tHis_Position\tAsp_Position\tConserved_Triad_Present\n")

# 读取已知和未知序列
known_seqs = read_fasta(KNOWN_FASTA)
unknown_seqs = read_fasta(UNKNOWN_FASTA)

print(f" - 共发现 {len(unknown_seqs)} 个未知序列")

for idx, (unknown_id, unknown_seq) in enumerate(unknown_seqs.items()):
    print(f"\n🚀 m   处理第 {idx+1}/{len(unknown_seqs)} 个蛋白: {unknown_id}")

    # 创建临时输入文件：合并未知 + 已知序列
    temp_input_seqs = {unknown_id: unknown_seq, **known_seqs}
    write_fasta(temp_input_seqs, TEMP_INPUT)

    # 运行 MAFFT 比对
    aligned_file = OUTPUT_DIR / f"{unknown_id}_aligned.fasta"
    run_mafft(TEMP_INPUT, aligned_file)

    # 分析保守三联体
    has_triad, ser_pos, his_pos, asp_pos = check_unknown_triad(aligned_file, unknown_id)
    conclusion = "Yes" if has_triad else "No"

    # 保存到摘要文件
    with open(SUMMARY_FILE, "a") as f:
        f.write(f"{unknown_id}\t{ser_pos}\t{his_pos}\t{asp_pos}\t{conclusion}\n")

    print(f" - 是否具有保守三联体: {conclusion}")
    print(f" - 保守位点: Ser@{ser_pos}, His@{his_pos}, Asp@{asp_pos}")

# 清理临时文件
os.remove(TEMP_INPUT)

print("\n🎉 全部处理完成！")
print(f"📊 结果摘要文件保存至: {SUMMARY_FILE}")
print(f"🧬 比对结果保存至: {OUTPUT_DIR}")