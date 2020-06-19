# Lua 基础数据类型

1, 函数 type 能够返回一个值或一个变量所属的类型。

2, 一个变量在第一次赋值前的默认值是 nil，将 nil 赋予给一个全局变量就等同于删除它。

3, **lua 中 nil 和 false 为“假”，其它所有值均为“真”**

4, 字符串: 4 级反的长括号写作 ]====]
 * 一个长字符串可由任何一级的正的长括号开始，而由第一个碰到的同级反的长括号结束 
 * 词法分析过程将不受分行限制，不处理任何转义符，并且忽略掉任何不同级别的长括号
     * 在 Lua 实现中，两个完全一样的 Lua 字符串在 Lua 虚拟机中只会存储一份。
     * 每一个 Lua 字符串在创建时都会插入到 Lua 虚拟机内部的一个全局的哈希表中。 

5,table (表)  “关联数组”
```lua
local corp = {
    web = "www.google.com",   --索引为字符串，key = "web",
                              --            value = "www.google.com"
    telephone = "12345678",   --索引为字符串
    staff = {"Jack", "Scott", "Gary"}, --索引为字符串，值也是一个表
    100876,              --相当于 [1] = 100876，此时索引为数字
                         --      key = 1, value = 100876
    100191,              --相当于 [2] = 100191，此时索引为数字
    [10] = 360,          --直接把数字索引给出
    ["city"] = "Beijing" --索引为字符串
}
```

6 function (函数)

  有名函数的定义本质上是匿名函数对变量的赋值。
```lua
function foo()
end

--等价于
foo = function ()
end
```

# 表达式

1 **lua 语言中“不等于”运算符的写法为：~=**

2 Lua 中的 and 和 or 是不同于 c 语言的

* a and b 如果 a 为 nil，则返回 a，否则返回 b;
* a or b 如果 a 为 nil，则返回 b，否则返回 a。

```lua
local c = nil
local d = 0
local e = 100
print(c and d)  -->打印 nil
print(c and e)  -->打印 nil
print(d and e)  -->打印 100
print(c or d)   -->打印 0
print(c or e)   -->打印 100
print(not c)    -->打印 true
print(not d)    -->打印 false

```

3 字符串连接

**注意，连接操作符只会创建一个新字符串，而不会改变原操作数。**

* 在 Lua 中连接两个字符串，可以使用操作符“..”（两个点）  
* 如果其任意一个操作数是数字的话，Lua 会将这个数字转换成字符串。
* 也可以使用 string 库函数 string.format 连接字符串。

**Lua 字符串本质上是只读的**

# 控制结构

1 Lua 并没有像许多其他语言那样提供 continue

2 repeat 类似于do-while，但是控制方式是刚好相反的
* 简单点说，执行 repeat 循环体后，直到 until 的条件为真时才结束

# Lua 函数

数既可以完成某项特定的任务，也可以只做一些计算并返回结果。  
在第一种情况中，一句函数调用被视为一条语句；而在第二种情况中，则将其视为一句表达式。


由于全局变量一般会污染全局名字空间，同时也有性能损耗（即查询全局环境表的开销），  
因此我们应当尽量使用“局部函数”，其记法是类似的，**只是开头加上 local 修饰符**

1  函数的参数 : 参数大部分是按值传递的

2  若形参个数和实参个数不同时，Lua 会自动调整实参个数
  * 规则：
      * 若实参个数大于形参个数，从左向右，多余的实参被忽略；
      * 若实参个数小于形参个数，从左向右，没有被实参初始化的形参会被初始化为 nil。

3 变长参数
```lua
local function func( ... )                -- 形参为 ... ,表示函数采用变长参数

   local temp = {...}                     -- 访问的时候也要使用 ...
   local ans = table.concat(temp, " ")    -- 使用 table.concat 库函数对数
                                          -- 组内容使用 " " 拼接成字符串。
   print(ans)
end

func(1, 2)        -- 传递了两个参数
func(1, 2, 3, 4)  -- 传递了四个参数

-->output
1 2

1 2 3 4
```

4 **在常用基本类型中，除了 table 是按址传递类型外，其它的都是按值传递参数**

5  Lua允许函数返回多个值

6  全动态函数调用

调用回调函数，并把一个数组参数作为回调函数的参数。
```lua
local args = {...} or {}
method_name(unpack(args, 1, table.maxn(args)))
```
使用场景
* 你要调用的函数参数是未知的；
* 函数的实际参数的类型和数目也都是未知的。


# 模块

一个代码模块就是一个程序库，可以通过 require 来加载
加载后的结果通过是一个 Lua table，这个表就像是一个命名空间，其内容就是模块中导出的所有东西

# String 库

**下标是从 1 开始的**

```lua
--- string.byte(s [, i [, j ]])
--- 返回字符 s[i]、s[i + 1]、s[i + 2]、······、s[j] 所对应的 ASCII 码

print(string.byte("abc", 1, 3))
print(string.byte("abc", 3)) -- 缺少第三个参数，第三个参数默认与第二个相同，此时为 3
print(string.byte("abc"))    -- 缺少第二个和第三个参数，此时这两个参数都默认为 1

-->output
97    98    99
99
97

```
```lua
--- string.char (...)

print(string.char(96, 97, 98))
print(string.char())        -- 参数为空，默认是一个0，
                            -- 你可以用string.byte(string.char())测试一下
print(string.char(65, 66))

--> output
`ab

AB

```
```lua
string.upper(s)  string.lower(s) ---大小写


--- 返回 p 字符串在 s 字符串中出现的开始位置和结束位置；
string.find(s, p [, init [, plain]])


local find = string.find
print(find("abc cba", "ab"))
print(find("abc cba", "ab", 2))     -- 从索引为2的位置开始匹配字符串：ab
print(find("abc cba", "ba", -1))    -- 从索引为7的位置开始匹配字符串：ba
print(find("abc cba", "ba", -3))    -- 从索引为5的位置开始匹配字符串：ba
print(find("abc cba", "(%a+)", 1))  -- 从索引为1处匹配最长连续且只含字母的字符串
print(find("abc cba", "(%a+)", 1, true)) --从索引为1的位置开始匹配字符串：(%a+)

-->output
1   2
nil
nil
6   7
1   3   abc
nil



```

```lua
string.format(formatstring, ...)

print(string.format("%.4f", 3.1415926))     -- 保留4位小数
print(string.format("%d %x %o", 31, 31, 31))-- 十进制数31转换成不同进制
d = 29; m = 7; y = 2015                     -- 一行包含几个语句，用；分开
print(string.format("%s %02d/%02d/%d", "today is:", d, m, y))

-->output
3.1416
31 1f 37
today is: 29/07/2015

```

```lua
--- 返回目标字符串中与模式匹配的子串；
string.match(s, p [, init])

print(string.match("hello lua", "lua"))
print(string.match("lua lua", "lua", 2))  --匹配后面那个lua
print(string.match("lua lua", "hello"))
print(string.match("today is 27/7/2015", "%d+/%d+/%d+"))

-->output
lua
lua
nil
27/7/2015


--- string.match 目前并不能被 JIT 编译，
--- 应尽量使用 ngx_lua 模块提供的 ngx.re.match 等接口。
```

```lua

--- 返回一个迭代器函数，通过这个迭代器函数可以遍历到在字符串 s 中出现模式串 p 的所有地方。
string.gmatch(s, p)

s = "hello world from Lua"
for w in string.gmatch(s, "%a+") do  --匹配最长连续且只含字母的字符串
    print(w)
end

-->output
hello
world
from
Lua


t = {}
s = "from=world, to=Lua"
for k, v in string.gmatch(s, "(%a+)=(%a+)") do  --匹配两个最长连续且只含字母的
    t[k] = v                                    --字符串，它们之间用等号连接
end
for k, v in pairs(t) do
print (k,v)
end

-->output
to      Lua
from    world

```
```lua

--- 返回字符串 s 中，索引 i 到索引 j 之间的子字符串。
string.sub(s, i [, j])


---将目标字符串 s 中所有的子串 p 替换成字符串 r。
---可选参数 n，表示限制替换次数。
---返回值有两个，第一个是被替换后的字符串，第二个是替换了多少次。
string.gsub(s, p, r [, n])

```

# table 库
**在 Lua 中，数组下标从 1 开始计数**

table.getn 获取长度

* 对于常规的数组，里面从 1 到 n 放着一些非空的值的时候，它的长度就精确的为 n，即最后一个值的下标\
* **任何一个 nil 值都有可能被当成数组的结束**


```lua
--- 返回 table[i]..sep..table[i+1] ··· sep..table[j] 连接成的字符串。
--- 填充字符串 sep 默认为空白字符串。
--- 如果 i 大于 j，返回一个空字符串。
table.concat (table [, sep [, i [, j ] ] ])

local a = {1, 3, 5, "hello" }
print(table.concat(a))              -- output: 135hello
print(table.concat(a, "|"))         -- output: 1|3|5|hello
print(table.concat(a, " ", 4, 2))   -- output:
print(table.concat(a, " ", 2, 4))   -- output: 3 5 hello


--- 在（数组型）表 table 的 pos 索引位置插入 value
table.insert (table, [pos ,] value)
--- 在表 table 中删除索引为 pos（pos 只能是 number 型）的元素，并返回这个被删除的元素
table.remove (table [, pos])


```

# [文件操作](https://moonbingbing.gitbooks.io/openresty-best-practices/content/lua/file.html)

隐式文件描述，显式文件描述。
这些文件 I/O 操作，在 OpenResty 的上下文中对事件循环是会产生阻塞效应

实际中的应用，在 OpenResty 项目中应尽可能让网络处理部分、文件 I/0 操作部分相互独立，不要揉和在一起。

**隐式文件描述**

```lua
--- 设置一个默认的输入或输出文件，然后在这个文件上进行所有的输入或输出操作。
--- 所有的操作函数由 io 表提供。


file = io.input("test1.txt")    -- 使用 io.input() 函数打开文件

repeat
    line = io.read()            -- 逐行读取内容，文件结束时返回nil
    if nil == line then
        break
    end
    print(line)
until (false)

io.close(file)                  -- 关闭文件

--> output
my test file
hello
lua


```
**显式文件描述**
```lua
--- 使用 file:XXX() 函数方式进行操作, 其中 file 为 io.open() 返回的文件句柄。

file = io.open("test2.txt", "r")    -- 使用 io.open() 函数，以只读模式打开文件

for line in file:lines() do         -- 使用 file:lines() 函数逐行读取文件
   print(line)
end

file:close()

-->output
my test2
hello lua

```