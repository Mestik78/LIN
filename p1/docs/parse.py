import sys, re
with open("Práctica 1_ Módulos del kernel Linux.html", "r") as f:
    html = f.read()
text = re.sub('<[^<]+>', '\n', html)
lines = [l.strip() for l in text.split('\n') if l.strip()]
for i, line in enumerate(lines):
    if "read" in line.lower() or "dos veces" in line.lower() or "cat" in line.lower():
        print(line)
