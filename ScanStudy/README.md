并行扫描求前缀和

```
A.init=0, B.init=0
scan(A), scan(B)

B.init = A.ans
C.init = A.ans + B.ans
scan(B), scan(C)
```

薄板可见

![sight](sight.jpg)
