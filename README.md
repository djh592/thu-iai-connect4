# 四子棋实验

## 策略说明

我使用了蒙特卡洛搜索算法来实现我的对抗策略。

在我的代码中，我定义了蒙特卡洛树类，构造蒙特卡洛树时，会复制当前棋盘的所有信息。我设计的蒙特卡洛树是多叉树的结构，但是它并不直接通过指针在节点之间切换，而是在切换节点位置的同时在复制的棋盘上模拟下棋的操作（见 `step` 和 `back`），这样就不用在每个节点上存储局面信息，造成过度的存储，并且按顺序访问到某一节点时，棋局也会更新到相应的状态。

蒙特卡洛树扩展路径的依据是 Upper Confidence Bound(UCB)，UCB 的计算公式和课件中相同，常数因子的选择是 0.8；扩展到树的末端时，会从当前的状态开始模拟对局，模拟出结果后就会将结果一步步上传，传回 root 时所有节点更新，并且棋盘回到原始状态。

为了让路径搜索更加高效和准确，我对拓展路径的过程进行了改良，尽可能避免冗余的分支。对于当前的玩家：

- 如果当前局面下有一种必胜的落子点，则将这一点设置为唯一的子节点，不考虑其他分支。
- 统计当前局面下对手的必胜点数：如果为 0，正常拓展分支；如果为 1，将这个落子点设置为唯一分支（即，必须堵截对方）；如果大于 1，直接宣告失败。

此外，我还做了一些其他的优化，比如将可选的分支打乱，避免优先拓展左侧分支；存储 UCB，避免重复运算 log 和 sqrt；为了更好地控制时间，我将上一步蒙特卡洛树的析构放到本次的构造中，从而将删除树的成本计入决策的 3 秒钟；我还维护了删除节点的列表，这样可以快速地删除整棵树，不用在各个节点之间游走，一一析构。

## 对抗结果

与给出的50个样本策略进行对抗，测试得到的结果如下：

```
My wins: 95
Opponent wins: 5
Ties: 0
```

从测试的结果来看，我的 AI 胜率是 95%，总共有 5 次失败，0 次平局。

更具体地讲，我设计的这个算法在对抗编号小于 90 的样本策略时都可以比较轻松地取胜；在对抗编号 90 以上的样本策略时会有落败的情况，在我的 5 次失败发生在 94、96、100这三个点。

最终提交到平台上测试的结果如下，胜率是 99%，平台上运行似乎总是比我本地运行有更好的结果，也可能是最终的测试比较简单。。。

![alt text](image.png)

## 其他潜在的优化点

我已经判断了我方是否必胜/必输和是否有必须要堵的点，其实还可以顺带判断是否会给敌人创造必胜点，只需要在筛选选一下每次选出的子节点即可。

具体来讲，筛选的方法就是先更新到子节点的状态，然后让对手在你刚刚下棋的位置上方落子（因为其他位置的落子已经排除掉了）。如果对手这时赢了，那么这个点就是会给对方创造必胜机会的点，也要删除。

加上这一策略后，`get_children` 函数会变成这个样子：

```cpp
void MCT::get_children(Node *node)
{
    Player player = node->player == USER ? MACHINE : USER;
    if (player == USER)
    {
        for (int i = 0; i < N; i++)
        {
            if (top[i])
            {
                int y = i, x = top[y] - 1;
                board[x][y] = 1;
                top[y] -= (y == noY && x == noX + 1) ? 2 : 1;
                if (userWin(x, y, M, N, board)) // 能直接赢
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    while (node->child_num)
                        delete node->child_list[--node->child_num];
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    node->state = RESULT_LOSE; // 父节点输了 => 子节点赢了
                    return;
                }
                else if (isTie(N, top)) // 平局
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    while (node->child_num)
                        delete node->child_list[--node->child_num];
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    node->state = RESULT_FAIR;
                    return;
                }
                else
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                }
            }
        }
        death_pos_size = 0; // 对方能直接赢的位置
        for (int i = 0; i < node->child_num; i++)
        {
            Node *child = node->child_list[i];
            board[child->lastX][child->lastY] = 2;
            if (machineWin(child->lastX, child->lastY, M, N, board)) // 对方能直接赢
                death_pos[death_pos_size++] = child;
            board[child->lastX][child->lastY] = 0;
        }
        // 0个：继续；1个：必须堵住；2个以上：已经输了
        if (death_pos_size)
        {
            for (int i = 0; i < node->child_num; i++)
                if (std::find(death_pos, death_pos + death_pos_size, node->child_list[i]) == death_pos + death_pos_size)
                    delete node->child_list[i];
                else
                    node->child_list[i] = nullptr;
            node->child_num = 0;
            for (int i = 0; i < death_pos_size; i++)
                node->child_list[node->child_num++] = death_pos[i];
            if (node->child_num >= 1)
                node->state = RESULT_WIN;
            return;
        }
        else
        {
            for (int i = 0; i < node->child_num; i++)
            {
                int x = node->child_list[i]->lastX;
                int y = node->child_list[i]->lastY;
                if (x - 1 >= 0)
                {
                    board[x][y] = 1;
                    board[x - 1][y] = 2;
                    if (machineWin(x - 1, y, M, N, board)) // 对方能直接赢
                    {
                        std::swap(node->child_list[i], node->child_list[node->child_num - 1]);
                        delete node->child_list[--node->child_num];
                    }
                    board[x][y] = 0;
                    board[x - 1][y] = 0;
                }
            }
            if (node->child_num >= 1)
                std::shuffle(node->child_list, node->child_list + node->child_num, rng); // 打乱顺序
            else
                node->state = RESULT_WIN;
        }
    }
    else
    {
        for (int i = 0; i < N; i++)
        {
            if (top[i])
            {
                int y = i, x = top[y] - 1;
                board[x][y] = 2;
                top[y] -= (y == noY && x == noX + 1) ? 2 : 1;
                if (machineWin(x, y, M, N, board)) // 能直接赢
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    while (node->child_num)
                        delete node->child_list[--node->child_num];
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    node->state = RESULT_LOSE; // 父节点输了 => 子节点赢了
                    return;
                }
                else if (isTie(N, top))
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    while (node->child_num)
                        delete node->child_list[--node->child_num];
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    node->state = RESULT_FAIR;
                    return;
                }
                else
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                }
            }
        }
        death_pos_size = 0; // 对方能直接赢的位置
        for (int i = 0; i < node->child_num; i++)
        {
            Node *child = node->child_list[i];
            board[child->lastX][child->lastY] = 1;
            if (userWin(child->lastX, child->lastY, M, N, board)) // 对方能直接赢
                death_pos[death_pos_size++] = child;
            board[child->lastX][child->lastY] = 0;
        }
        // 0个：继续；1个：必须堵住；2个以上：已经输了
        if (death_pos_size)
        {
            for (int i = 0; i < node->child_num; i++)
                if (std::find(death_pos, death_pos + death_pos_size, node->child_list[i]) == death_pos + death_pos_size)
                    delete node->child_list[i];
                else
                    node->child_list[i] = nullptr;
            node->child_num = 0;
            for (int i = 0; i < death_pos_size; i++)
                node->child_list[node->child_num++] = death_pos[i];
            if (node->child_num >= 1)
                node->state = RESULT_WIN;
            return;
        }
        else
        {
            for (int i = 0; i < node->child_num; i++)
            {
                int x = node->child_list[i]->lastX;
                int y = node->child_list[i]->lastY;
                if (x - 1 >= 0)
                {
                    board[x][y] = 2;
                    board[x - 1][y] = 1;
                    if (userWin(x - 1, y, M, N, board)) // 对方能直接赢
                    {
                        std::swap(node->child_list[i], node->child_list[node->child_num - 1]);
                        delete node->child_list[--node->child_num];
                    }
                    board[x][y] = 0;
                    board[x - 1][y] = 0;
                }
            }
            if (node->child_num >= 1)
                std::shuffle(node->child_list, node->child_list + node->child_num, rng); // 打乱顺序
            else
                node->state = RESULT_WIN;
        }
    }
}
```

注：这份代码可能与当前版本不匹配。

虽然复杂度并没有增加多少，但是代码会变得十分臃肿，所以我没有考虑将它加入我的最终策略中。