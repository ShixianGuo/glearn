##  [面试题03. 数组中重复的数字](https://leetcode-cn.com/problems/shu-zu-zhong-zhong-fu-de-shu-zi-lcof/)

思路: 把数组视为哈希表
```

由于数组元素的值都在指定的范围内，这个范围恰恰好与数组的下标可以一一对应；

因此看到数值，就可以知道它应该放在什么位置，这里数字nums[i] 应该放在下标为 i 的位置上，这就像是我们人为编写了哈希函数，这个哈希函数的规则还特别简单；

而找到重复的数就是发生了哈希冲突；

```
<div align="center"> <img src="pic/JZ03.png"/> </div>

分析：这个思路利用到了数组的元素值的范围恰好和数组的长度是一样的，因此数组本身可以当做哈希表来用。遍历一遍就可以找到重复值，但是修改了原始数组。

```cpp

class Solution {
public:
	void swap(int& a,int& b) {
		int tmp = a;
		a = b;
		b = tmp;
	}

	int findRepeatNumber(vector<int>& nums) {

		for (int i = 0; i < nums.size(); i++){
			// 如果当前的数 nums[i] 没有在下标为 i 的位置上，就把它交换到下标 i 上
	        // 交换过来的数还得做相同的操作，因此这里使用 while
			while (nums[i]!=i)	{
				if (nums[i]==nums[nums[i]]) {
					// 如果下标为 nums[i] 的数值 nums[nums[i]] 它们二者相等
					// 正好找到了重复的元素，将它返回
					return nums[i];
				}
				swap(nums[i], nums[nums[i]]);
			}
		}
		return -1;
	}
};

```


##  [面试题04. 二维数组中的查找](https://leetcode-cn.com/problems/er-wei-shu-zu-zhong-de-cha-zhao-lcof/)
算法思想
```
标志数引入： 此类矩阵中左下角和右上角元素有特殊性，称为标志数。
  左下角元素： 为所在列最大元素，所在行最小元素。 
  右上角元素： 为所在行最大元素，所在列最小元素。

标志数性质： 将 matrix 中的左下角元素（标志数）记作 flag ，则有:
若 flag > target ，则 target 一定在 flag 所在行的上方，即 flag 所在行可被消去。
若 flag < target ，则 target 一定在 flag 所在列的右方，即 flag 所在列可被消去。
本题解以左下角元素为例，同理，右上角元素 也具有行（列）消去的性质。
算法流程： 根据以上性质，设计算法在每轮对比时消去一行（列）元素，以降低时间复杂度。

从矩阵 matrix 左下角元素（索引设为 (i, j) ）开始遍历，并与目标值对比：
当 matrix[i][j] > target 时： 行索引向上移动一格（即 i--），即消去矩阵第 i 行元素；
当 matrix[i][j] < target 时： 列索引向右移动一格（即 j++），即消去矩阵第 j 列元素；
当 matrix[i][j] == target 时： 返回 truetrue 。
若行索引或列索引越界，则代表矩阵中无目标值，返回 falsefalse 。

```

<div align="center"> <img src="pic/JZ04.png"/> </div>

算法本质： 每轮 i 或 j 移动后，相当于生成了“消去一行（列）的新矩阵”， 索引(i,j) 指向新矩阵的左下角元素（标志数），因此可重复使用以上性质消去行（列）

```cpp


class Solution {
public:
	bool findNumberIn2DArray(vector<vector<int>>& matrix, int target) {
		int i = matrix.size() - 1, j = 0;

		while (i>=0&&j<matrix[0].size()){
			if (matrix[i][j] > target) i--;
			else if (matrix[i][j] < target)j++;
			else  return true;

		}

		return false;

	}
};
```

## [面试题05. 替换空格](https://leetcode-cn.com/problems/ti-huan-kong-ge-lcof/)

```cpp
class Solution {
public:
    string replaceSpace(string s) {
        string res;
        for(auto c : s){
            if(c == ' ')
                res += "%20";
            else
                res += c;
        }
        return res;
    }
};
```

## [面试题06. 从尾到头打印链表](https://leetcode-cn.com/problems/cong-wei-dao-tou-da-yin-lian-biao-lcof/)

```
class Solution {
public:
    vector<int> reversePrint(ListNode* head) {
        vector<int> res;
        while (head){
            res.push_back(head->val);
            head = head->next;
        }
        reverse(res.begin(), res.end());
        return res;
    }
};

```

## [面试题07. 重建二叉树](https://leetcode-cn.com/problems/zhong-jian-er-cha-shu-lcof/)

题目分析：

> 前序遍历特点： 节点按照 [ 根节点 | 左子树 | 右子树 ] 排序，以题目示例为例：[ 3 | 9 | 20 15 7 ]
中序遍历特点： 节点按照 [ 左子树 | 根节点 | 右子树 ] 排序，以题目示例为例：[ 9 | 3 | 15 20 7 ]
根据题目描述输入的前序遍历和中序遍历的结果中都不含重复的数字，其表明树中每个节点值都是唯一的。

根据以上特点，可以按顺序完成以下工作：

> 前序遍历的首个元素即为根节点 root 的值；
在中序遍历中搜索根节点 root 的索引 ，可将中序遍历划分为 [ 左子树 | 根节点 | 右子树 ] 。
根据中序遍历中的左（右）子树的节点数量，可将前序遍历划分为 [ 根节点 | 左子树 | 右子树 ] 。
自此可确定 三个节点的关系 ：1.树的根节点、2.左子树根节点、3.右子树根节点（即前序遍历中左（右）子树的首个元素）。
> 子树特点： 子树的前序和中序遍历仍符合以上特点，以题目示例的右子树为例：前序遍历：[20 | 15 | 7]，中序遍历 [ 15 | 20 | 7 ] 。
> 根据子树特点，我们可以通过同样的方法对左（右）子树进行划分，每轮可确认三个节点的关系 。此递推性质让我们联想到用 递归方法 处理。


递归解析：

> 递推参数： 前序遍历中根节点的索引pre_root、中序遍历左边界in_left、中序遍历右边界in_right。
终止条件： 当 in_left > in_right ，子树中序遍历为空，说明已经越过叶子节点，此时返回 nullnull 。
递推工作：
建立根节点root： 值为前序遍历中索引为pre_root的节点值。
搜索根节点root在中序遍历的索引i： 为了提升搜索效率，本题解使用哈希表 dic 预存储中序遍历的值与索引的映射关系，每次搜索的时间复杂度为 O(1)O(1)。
构建根节点root的左子树和右子树： 通过调用 recur() 方法开启下一层递归。
左子树： 根节点索引为 pre_root + 1 ，中序遍历的左右边界分别为 in_left 和 i - 1。
右子树： 根节点索引为 i - in_left + pre_root + 1（即：根节点索引 + 左子树长度 + 1），中序遍历的左右边界分别为 i + 1 和 in_right。
返回值： 返回 root，含义是当前递归层级建立的根节点 root 为上一递归层级的根节点的左或右子节点。

```cpp
class Solution {
public:

	//利用原理,先序遍历的第一个节点就是根。在中序遍历中通过根 区分哪些是左子树的，哪些是右子树的
	unordered_map<int, int>dic; //标记中序遍历
	vector<int> preorder; //保留的先序遍历

	TreeNode* recur(int pre_root,int in_left,int in_right) {

		//终止条件
		if (in_left > in_right) {
			return nullptr;
		}

		TreeNode* root = new TreeNode(preorder[pre_root]);

		int idx = dic[preorder[pre_root]];


		//左子树的根节点就是 左子树的(前序遍历）第一个，就是+1,左边边界就是left，右边边界是中间区分的idx-1
		root->left = recur(pre_root+1,in_left,idx-1);

		//右子树的根，就是右子树（前序遍历）的第一个,就是当前根节点 加上左子树的数量
		// pre_root_idx 当前的根 = 左子树的长度 = 左子树的左边-右边 (idx-1 - in_left_idx +1) 。最后+1就是右子树的根了
		root->right = recur(pre_root+(idx-1-in_left+1)+1  ,idx+1,in_right);
		 
		return root;

	}

	TreeNode* buildTree(vector<int>& preorder, vector<int>& inorder) {
		this->preorder = preorder;
		for (int i = 0; i < inorder.size(); i++){
			dic.insert(pair<int,int>(inorder[i],i));
		}

		return recur(0,0,inorder.size()-1);

	}
};


```
## [面试题09. 用两个栈实现队列](https://leetcode-cn.com/problems/yong-liang-ge-zhan-shi-xian-dui-lie-lcof/)


解题思路

算法分为入队和出队过程。

入队过程：将元素放入 inStack 中。

出队过程：

outStack 不为空：弹出元素
outStack 为空：将 inStack 元素依次弹出，放入到 outStack 中（在数据转移过程中，顺序已经从后入先出变成了先入先出）

```cpp

class CQueue {
public:
	CQueue() {

	}

	void appendTail(int value) {
		instack.push(value);
	}

	int deleteHead() {
		if (outstack.empty()) {
			if (instack.empty()) {
				return -1;
  			}
			else
			{
				while (!instack.empty())
				{
					outstack.push(instack.top());
					instack.pop();
				}
			}
		}

		int top = outstack.top();
		outstack.pop();

		return top;

	}

	stack<int>instack;
	stack<int>outstack;
};
```

## [面试题10- I. 斐波那契数列](https://leetcode-cn.com/problems/fei-bo-na-qi-shu-lie-lcof/)

原理： 以斐波那契数列性质 f(n + 1) = f(n) + f(n - 1)f(n+1)=f(n)+f(n−1) 为转移方程。

大数越界： 随着 nn 增大, f(n)f(n) 会超过 Int32 甚至 Int64 的取值范围，导致最终的返回值错误。

```cpp
class Solution {
public:
	//f(n + 1) =f(n)+f(n−1) 
	int fib(int n) {
		if (n == 1) return 1;
		if (n == 0)return 0;

		int f0 = 0;
		int f1 = 1;

		int fn;

		for (int i = 2; i <= n; i++)
		{
			fn = (f1 + f0)% 1000000007;
            f0 = f1;
			f1 = fn;
			
		}

		return fn;
	}
};
```
## [面试题10- II. 青蛙跳台阶问题](https://leetcode-cn.com/problems/qing-wa-tiao-tai-jie-wen-ti-lcof/)


设跳上 n 级台阶有 f(n) 种跳法。在所有跳法中，青蛙的最后一步只有两种情况： 跳上 1级或 2 级台阶
当为 1 级台阶： 剩 n-1 个台阶，此情况共有 f(n-1) 种跳法；
当为 2 级台阶： 剩 n-2 个台阶，此情况共有 f(n-2) 种跳

f(n)=f(n−1)+f(n−2) 

<div align="center"> <img src="pic/JZ10-2.png"/> </div>

```cpp
class Solution {
public:
	//f(n)=f(n−1)+f(n−2) 
	int numWays(int n) {
		
		if (n <= 1) return 1;
		if (n == 2) return 2;

		int f0 = 1;
		int f1 = 2;

		int fn;

		for (int i = 2; i < n; i++)
		{
			fn = (f1 + f0) % 1000000007;
			f0 = f1;
			f1 = fn;
		}

		return fn;
	}
};
```
## [面试题11. 旋转数组的最小数字](https://leetcode-cn.com/problems/xuan-zhuan-shu-zu-de-zui-xiao-shu-zi-lcof/)

解题思路：

如下图所示，寻找旋转数组的最小元素即为寻找 右排序数组 的首个元素 numbers[x] ，称 x 为 旋转点 。  
排序数组的查找问题首先考虑使用 二分法 解决，其可将遍历法的 线性级别 时间复杂度降低至 对数级别 
出处。 

<div align="center"> <img src="pic/JZ11.png"/> </div>


