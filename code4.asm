bits 16

add bx, -4999
add si, 2
add bp, 2
add cx, 8
sub si, 2
sub bp, 2
sub cx, 8
add word [bp + si + 1000], 29
add byte [bx], 34
sub word [bx + di], 29
sub byte [bx], 34
cmp si, 2
cmp bp, 2
cmp cx, 8
cmp byte [bx], 34
cmp word [4834], 29

add ax, 1000
sub ax, 1000
cmp ax, 1000
add al, -30
add al, 9
sub al, -30
sub al, 9
cmp al, -30
cmp al, 9

add bx, [bx+si]
add bx, [bp]
add bx, [bp + 0]
add cx, [bx + 2]
add bh, [bp + si + 4]
add di, [bp + di + 6]
add [bx+si], bx
add [bp], bx
add [bp + 0], bx
add [bx + 2], cx
add [bp + si + 4], bh
add [bp + di + 6], di
add ax, [bp]
add al, [bx + si]
add ax, bx
add al, ah
 
sub bx, [bx+si]
sub bx, [bp]
sub bx, [bp + 0]
sub cx, [bx + 2]
sub bh, [bp + si + 4]
sub di, [bp + di + 6]
sub [bx+si], bx
sub [bp], bx
sub [bp + 0], bx
sub [bx + 2], cx
sub [bp + si + 4], bh
sub [bp + di + 6], di
sub ax, [bp]
sub al, [bx + si]
sub ax, bx
sub al, ah

cmp bx, [bx+si]
cmp bx, [bp]
cmp bx, [bp + 0]
cmp cx, [bx + 2]
cmp bh, [bp + si + 4]
cmp di, [bp + di + 6]
cmp [bx+si], bx
cmp [bp], bx
cmp [bp + 0], bx
cmp [bx + 2], cx
cmp [bp + si + 4], bh
cmp [bp + di + 6], di
cmp ax, [bp]
cmp al, [bx + si]
cmp ax, bx
cmp al, ah

test_label0:
jnz test_label1
jnz test_label0
test_label1:
jnz test_label0
jnz test_label1
 
label:
je label
jl label
jle label
jb label
jbe label
jp label
jo label
js label
jne label
jnl label
jg label
jnb label
ja label
jnp label
jno label
jns label
loop label
loopz label
loopnz label
jcxz label
