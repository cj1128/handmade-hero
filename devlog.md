# Dev log

## Hyperthread

CPU 内部有很多工作单元，叫做 Port。

某个 Thread 的指令需要占据某个 Port 并且需要一定的时间来完成，那么在这之间，其他 Port 就空闲了。

Hyperthreading 就是 CPU 在硬件级别可以动态从两个 Thread 中选择指令进行执行，这样最大化利用 CPU。

还是只有一个 CPU，只不过在调度上做了一些功夫。

## 优化

Day122(36f3735) 我发现我的代码 CPU 使用率明显高于 Casey 的代码，我的是 15% 左右，而 Casey 的是 5% 左右。

我把 Casey 的 DrawRectangleQuickly 函数拷贝进来，编译以后发现 CPU 是 5% 左右，说明就是这个函数实现的问题。

对比输出，我发现 DebugCycleCounter_ProcessPixel 的调用次数相差非常大，所以我就研究为什么会这样。

这里其实是没有经验，误入了歧途。

调用次数的计数实际上是 GetClampedRectArea(fillRect) / 2，所以先确定 fillRect 的计算是否一致，发现完全一致。

这下就不知道怎么回事了，同样的函数，传了同样的参数，但是显示 call 次数却不一样。

实验来实验去，最后我发现，我把 for 循环里的内容完全注释掉，结果 call 次数也不一样。

到这里我才恍然大悟，肯定是 O2 的高度优化导致代码的顺序被编译器调整了，因此 call 次数出了偏差，如果改成 Od，就会发现 call 次数和 Casey 的是一样的。

TODO: 为什么 O2 会导致 call 次数不准？

接下来就很清楚了，直接看 DebugCycleCounter_DrawRectangleQuickly，会发现根本原因还是执行速度太慢，我的代码需要 40K cycle，而 Casey 的只需要 10K 不到。

接下来就是比对 Casey 的代码一点一点地找出具体是哪里影响了性能。

花了大量的时间以后我发现，核心问题是循环中对结构体的引用和对指针的引用。

before:

```c
for(int y = fillRect.minY; y < fillRect.maxY; y += 2) {
  for(int x = fillRect.minX; x < fillRect.maxX; x += 4) {

    u8 *ptr0 = (u8 *)(texture->memory) + Mi(ptr, 0);
    u8 *ptr1 = (u8 *)(texture->memory) + Mi(ptr, 1);
    u8 *ptr2 = (u8 *)(texture->memory) + Mi(ptr, 2);
    u8 *ptr3 = (u8 *)(texture->memory) + Mi(ptr, 3);
    ...
  }
}
```

after:

```c
i32 minX = fillRect.minX;
i32 maxX = fillRect.maxX;
i32 minY = fillRect.minY;
i32 maxY = fillRect.maxY;

void *textureMemory = texture->memory;

for(int y = minY; y < maxY; y += 2) {
  for(int x = minX; x < maxX; x += 4) {
    u8 *ptr0 = (u8 *)(textureMemory) + Mi(ptr, 0);
    u8 *ptr1 = (u8 *)(textureMemory) + Mi(ptr, 1);
    u8 *ptr2 = (u8 *)(textureMemory) + Mi(ptr, 2);
    u8 *ptr3 = (u8 *)(textureMemory) + Mi(ptr, 3);
    ...
  }
}
```

就这样一个改动，立刻从 40Ｋ　变成了 7K。

LESSON: 循环中对内存的引用要记得手动处理，MSVC 没有那么智能。
