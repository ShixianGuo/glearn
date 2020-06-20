# [元表](https://moonbingbing.gitbooks.io/openresty-best-practices/content/lua/metatable.html)

**元表 (metatable) 的表现行为类似于 C++ 语言中的操作符重载**

* setmetatable(table, metatable)：此方法用于为一个表设置元表。
* getmetatable(table)：此方法用于获取表的元表对象。


通过重载 "__add" 元方法来计算集合的并集实例
```lua
local set1 = {10, 20, 30}   -- 集合
local set2 = {20, 40, 50}   -- 集合

-- 将用于重载__add的函数，注意第一个参数是self
local union = function (self, another)
    local set = {}
    local result = {}

    -- 利用数组来确保集合的互异性
    for i, j in pairs(self) do set[j] = true end
    for i, j in pairs(another) do set[j] = true end

    -- 加入结果集合
    for i, j in pairs(set) do table.insert(result, i) end
    return result
end
setmetatable(set1, {__add = union}) -- 重载 set1 表的 __add 元方法

local set3 = set1 + set2
for _, j in pairs(set3) do
    io.write(j.." ")               -->output：30 50 20 40 10
end
```

# 面向对象编程

**类**

将函数和相关的数据放置于同一个表中就形成了一个对象。

请看文件名为 account.lua 的源码
```lua
local _M = {}

local mt = { __index = _M }

function _M.deposit (self, v)
    self.balance = self.balance + v
end

function _M.withdraw (self, v)
    if self.balance > v then
        self.balance = self.balance - v
    else
        error("insufficient funds")
    end
end

function _M.new (self, balance)
    balance = balance or 0
    return setmetatable({balance = balance}, mt)
end

return _M

--- 上面这段代码 "setmetatable({balance = balance}, mt)"，
--- 其中 mt 代表 { __index = _M } ，这句话值得注意。
--- 根据我们在元表这一章学到的知识，我们明白，setmetatable 将 _M 作为新建表的原型，
--- 所以在自己的表内找不到 'deposit'、'withdraw' 这些方法和变量的时候，
--- 便会到 __index 所指定的 _M 类型中去寻找。

```
**继承**
```lua
---------- s_base.lua
local _M = {}

local mt = { __index = _M }

function _M.upper (s)
    return string.upper(s)
end

return _M

---------- s_more.lua
local s_base = require("s_base")

local _M = {}
_M = setmetatable(_M, { __index = s_base })


function _M.lower (s)
    return string.lower(s)
end

return _M

---------- test.lua
local s_more = require("s_more")

print(s_more.upper("Hello"))   -- output: HELLO
print(s_more.lower("Hello"))   -- output: hello
```

# 局部变量

Lua 的设计有一点很奇怪，在一个 block 中的变量，如果之前没有定义过，那么认为它是一个全局变量，而不是这个 block 的局部变量。这一点和别的语言不同。容易造成不小心覆盖了全局同名变量的错误。

# 判断数组大小

Lua 中，数组的实现方式其实类似于 C++ 中的 map，对于数组中所有的值，都是以键值对的形式来存储（无论是显式还是隐式），Lua 内部实际采用哈希表和数组分别保存键值对、普通值，所以不推荐混合使用这两种赋值方式

尤其需要注意的一点是：Lua 数组中允许 nil 值的存在，但是数组默认结束标志却是 nil。这类比于 C 语言中的字符串，字符串中允许 '\0' 存在，但当读到 '\0' 时，就认为字符串已经结束了。

# 非空判断

有时候不小心引用了一个没有赋值的变量，这时它的值默认为 nil。如果对一个 nil 进行索引的话，会导致异常

在实际的工程代码中，我们很难这么轻易地发现我们引用了 nil 变量。因此，在很多情况下我们在访问一些 table 型变量时，需要先判断该变量是否为 nil，


# 虚变量

当一个方法返回多个值时，有些返回值有时候用不到，要是声明很多变量来一一接收，显然不太合适

Lua 提供了一个虚变量(dummy variable)的概念， 按照惯例以一个下划线（“_”）来命名，用它来表示丢弃不需要的数值，仅仅起到占位的作用。

```lua
-- string.find (s,p) 从string 变量s的开头向后匹配 string
-- p，若匹配不成功，返回nil，若匹配成功，返回第一次匹配成功
-- 的起止下标。

local start, finish = string.find("hello", "he") --start 值为起始下标，finish
                                                 --值为结束下标
print ( start, finish )                          --输出 1   2

local start = string.find("hello", "he")      -- start值为起始下标
print ( start )                               -- 输出 1


local _,finish = string.find("hello", "he")   --采用虚变量（即下划线），接收起
                                              --始下标值，然后丢弃，finish接收
                                              --结束下标值
print ( finish )                              --输出 2
print ( _ )                                   --输出 1, `_` 只是一个普通变量,我们习惯上不会读取它的值

```

# 抵制使用 module() 定义模块

比较推荐的模块定义方法是：

```lua
-- square.lua 长方形模块
local _M = {}           -- 局部的变量
_M._VERSION = '1.0'     -- 模块版本

local mt = { __index = _M }

function _M.new(self, width, height)
    return setmetatable({ width=width, height=height }, mt)
end

function _M.get_square(self)
    return self.width * self.height
end

function _M.get_circumference(self)
    return (self.width + self.height) * 2
end

return _M

```
引用示例代码
```lua
local square = require "square"

local s1 = square:new(1, 2)
print(s1:get_square())          --output: 2
print(s1:get_circumference())   --output: 6
```

另一个跟 Lua 的 module 模块相关需要注意的点是，当 lua_code_cache on 开启时，require 加载的模块是会被缓存下来的，这样我们的模块就会以最高效的方式运行，直到被显式地调用如下语句（这里有点像模块卸载）：

```lua
package.loaded["square"] = nil
```

# 点号与冒号操作符的区别

冒号操作会带入一个 self 参数，用来代表 自己。而点号操作，只是 内容 的展开。 

在函数定义时，使用冒号将默认接收一个 self 参数，而使用点号则需要显式传入 self 参数。

# FFI

允许从纯 Lua 代码调用外部 C 函数，使用 C 数据结构



# [什么是 JIT？](https://moonbingbing.gitbooks.io/openresty-best-practices/content/lua/what_jit.html)

LuaJIT 的运行时环境包括一个用手写汇编实现的 Lua 解释器和一个可以直接生成机器代码的 JIT 编译器

lua 代码在被执行之前总是会先被 lfn 成 LuaJIT 自己定义的字节码

一开始的时候，Lua 字节码总是被 LuaJIT 的解释器解释执行

LuaJIT 的解释器会在执行字节码时同时记录一些运行时的统计信息，比如每个 Lua 函数调用入口的实际运行次数，还有每个 Lua 循环的实际执行次数。当这些次数超过某个预设的阈值时，便认为对应的 Lua 函数入口或者对应的 Lua 循环足够的“热”，这时便会触发 JIT 编译器开始工作。

如果当前 Lua 代码路径上的所有的操作都可以被 JIT 编译器顺利编译，则这条编译过的代码路径便被称为一个“trace”，在物理上对应一个 trace 类型的 GC 对象（即参与 Lua GC 的对象）。