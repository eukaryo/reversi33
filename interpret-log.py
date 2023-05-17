import re
import sys

NUMBER = 33

numbers = set()
with open(f"bulky-cnf{NUMBER}cpp-sat-log.txt", "r") as f:
    for line in f:
        if line[0] != "v":
            continue
        line = line[2:].strip().split(" ")
        line = set([int(x) for x in line])
        assert len(numbers & line) == 0
        numbers |= line

def num2pos(num):
    y = num // 8
    x = num % 8
    return "abcdefgh"[x] + str(y + 1)

move_pos = dict()
with open(f"dimacs_literal_2_token_{NUMBER}.csv", "r") as f:
    for line in f:
        m = re.fullmatch(r"move_pos_turn([0-9]+)_([0-9]+):[0-9]+,([0-9]+)", line.strip())
        if m is not None:
            turn = int(m.group(1))
            bitpos = int(m.group(2))
            literal_idx = int(m.group(3))
            if turn not in move_pos.keys():
                move_pos[turn] = 0
            if literal_idx in numbers:
                move_pos[turn] += 1 << bitpos

kifu = ""
for k, v in move_pos.items():
    kifu += num2pos(v)
print(f"kifu={kifu}")
