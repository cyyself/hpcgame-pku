## 题面

*Hint: mmap写入空文件的处理方式：https://stackoverflow.com/questions/44553907/mmap-sigbus-error-and-initializing-the-file*

不要惊慌。你没有看错，这就是一道简单题。

啊偶，虽然在某些比赛中，被标注为“简单题”的题目往往并不简单，但是在这场比赛中，我们确实试图提供一些简单题，以让大家能够更容易地获得更多分数。

你需要用 `P` 个核心，把 `N` 个 32 位有符号整数数组中所有元素 + 1，并且求原来（未加一）的数字之和`(MOD 100001651)`并输出。

## 输入

从 `input.bin` 读入。这是一个二进制文件，用小端法储存储存，可以直接进行内存映射。（题目保证这些数字小于 $2^{30}$）

第一个参数是核心数 `P` ，表示当前程序运行平台的核心数。第二个参数是 `N` ，表示数据总数（在32位整数表示范围内）。

## 输出

输出到 `output.bin`。也是一个二进制文件，建议直接内存映射写入。

第一个数字是求和取模的结果，后面的数字是修改后的数组。

## 提交

提交一个 C++ 源程序即可。

编译命令：

```bash
g++ -o solution solution.cpp -O1 -fopenmp -std=c++11
```

### 一个可能有用的数据生成器

```php
#!/usr/bin/php
<?php

if ($argc != 4) {
    die("USAGE: {$argv[0]} <OUTPUT_PATH> <P> <N_RANGE>\n");
}

function i32_to_bytes(int $n): array
{
    $rslt = [];
    for ($i = 0; $i < 4; ++$i) {
        $rslt[] = $n & 255;
        $n >>= 8;
    }
    return $rslt;
}

function bytes_to_string(array $n): string
{
    $a = array_map(fn (int $num) => chr($num), $n);
    return join('', $a);
}

function i32_to_string(int $n): string
{
    return bytes_to_string(i32_to_bytes($n));
}

$f = fopen($argv[1], "w");

$p = (int) $argv[2];
$n = (1 << ((int) trim($argv[3])));
$n += $n + rand(0, $n / 2);
$part = 1;
$n = floor($n / $part) * $part;
echo "p={$p}, n={$n}" . PHP_EOL;

fwrite($f, i32_to_string($p));
fwrite($f, i32_to_string($n));
for ($i = 0; $i < ($n / $part); ++$i) {
    $m = i32_to_string(rand(0, 1 << 30));
    fwrite($f, $m);
}

fclose($f);

```
