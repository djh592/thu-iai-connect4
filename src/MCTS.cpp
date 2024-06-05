#include "MCTS.h"
#include "utils.h"
#include "Judge.h"
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <vector>
#include <iostream> // for debug

const double C = 1.41421356237 * 0.8; // UCB中的常数项

std::vector<Node *> del_list; // 用于存储需要删除的节点

Node *MCT::step(Node *node, int r)
{
    Node *child = node->child_list[r];
    board[child->lastX][child->lastY] = (child->player == USER) ? 1 : 2;
    if (child->lastY == noY && child->lastX == noX + 1)
        top[child->lastY] -= 2;
    else
        top[child->lastY] -= 1;
    return child;
}

Node *MCT::back(Node *node)
{
    board[node->lastX][node->lastY] = 0;
    if (node->lastY == noY && node->lastX == noX + 1)
        top[node->lastY] += 2;
    else
        top[node->lastY] += 1;
    return node->parent;
}

int death_pos_size = 0;
Node *death_pos[MAX_BOARD_SIZE];

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
                        --node->child_num;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    del_list.push_back(node->child_list[node->child_num - 1]);
                    node->state = RESULT_LOSE; // 父节点输了 => 子节点赢了
                    return;
                }
                else if (isTie(N, top)) // 平局
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    while (node->child_num)
                        --node->child_num;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    del_list.push_back(node->child_list[node->child_num - 1]);
                    node->state = RESULT_FAIR;
                    return;
                }
                else
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    del_list.push_back(node->child_list[node->child_num - 1]);
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
            node->child_num = 0;
            for (int i = 0; i < death_pos_size; i++)
                node->child_list[node->child_num++] = death_pos[i];
            if (node->child_num > 1)
                node->state = RESULT_WIN;
            return;
        }
        else
            std::shuffle(node->child_list, node->child_list + node->child_num, rng); // 打乱顺序
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
                        --node->child_num;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    del_list.push_back(node->child_list[node->child_num - 1]);
                    node->state = RESULT_LOSE; // 父节点输了 => 子节点赢了
                    return;
                }
                else if (isTie(N, top))
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    while (node->child_num)
                        --node->child_num;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    del_list.push_back(node->child_list[node->child_num - 1]);
                    node->state = RESULT_FAIR;
                    return;
                }
                else
                {
                    board[x][y] = 0;
                    top[y] += (y == noY && x == noX + 1) ? 2 : 1;
                    node->child_list[node->child_num++] = new Node(player, x, y, node);
                    del_list.push_back(node->child_list[node->child_num - 1]);
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
            node->child_num = 0;
            for (int i = 0; i < death_pos_size; i++)
                node->child_list[node->child_num++] = death_pos[i];
            if (node->child_num > 1)
                node->state = RESULT_WIN;
            return;
        }
        else
            std::shuffle(node->child_list, node->child_list + node->child_num, rng); // 打乱顺序
    }
}

int MCT::best_child(Node *node)
{
    double max_UCB = -1;
    int best_child = -1;
    for (int i = 0; i < node->child_num; i++)
    {
        Node *child = node->child_list[i];
        double cur_UCB = child->UCB;
        if (cur_UCB > max_UCB)
        {
            max_UCB = cur_UCB;
            best_child = i;
        }
    }
    if (best_child == -1)
    {
        best_child = 0;
        node->state = RESULT_LOSE;
        for (int i = 0; i < node->child_num; i++)
            if (node->child_list[i]->state < node->state)
            {
                best_child = i;
                node->state = node->child_list[i]->state;
            }
    }
    return best_child;
}

Node *MCT::select()
{
    Node *cur = root;
    while (cur->state == SELECT)
        cur = step(cur, best_child(cur));
    if (cur->state == EXPAND)
    {
        Node *child = step(cur, cur->expand_cnt++);
        get_children(child);
        if (cur->expand_cnt == cur->child_num)
            cur->state = SELECT;
        return child;
    }
    return cur;
}

int top_sim[MAX_BOARD_SIZE];
int list_size = 0;
int x_list[MAX_BOARD_SIZE * MAX_BOARD_SIZE], y_list[MAX_BOARD_SIZE * MAX_BOARD_SIZE];
int valid_size = 0;
int valid_pos_x[MAX_BOARD_SIZE], valid_pos_y[MAX_BOARD_SIZE];

Result MCT::simulate(Node *node)
{
    std::copy(top, top + N, top_sim);
    list_size = 0;                 // 记录每一步的坐标（用于回溯）
    int cur_player = node->player; // 当前下棋的玩家（应该是父节点）
    int x, y;                      // 当前下棋的位置
    Result result;
    while (true)
    {
        // 换人下棋
        cur_player = (cur_player == USER) ? MACHINE : USER;
        valid_size = 0;
        for (int i = 0; i < N; i++)
            if (top_sim[i])
            {
                valid_pos_y[valid_size] = i;
                valid_pos_x[valid_size] = top_sim[i] - 1;
                valid_size++;
            }
        if (valid_size)
        {
            int r = get_random(0, valid_size - 1);
            x = valid_pos_x[r], y = valid_pos_y[r];
        }
        else
        {
            result = FAIR;
            break;
        }
        board[x][y] = (cur_player == USER) ? 1 : 2;
        top_sim[y] -= (y == noY && x == noX + 1) ? 2 : 1;
        x_list[list_size] = x;
        y_list[list_size] = y;
        list_size++;
        // 检查是否有人赢了
        if (cur_player == USER && userWin(x, y, M, N, board))
        {
            result = node->player == USER ? WIN : LOSE;
            break;
        }
        else if (cur_player == MACHINE && machineWin(x, y, M, N, board))
        {
            result = node->player == MACHINE ? WIN : LOSE;
            break;
        }
        else if (isTie(N, top_sim))
        {
            result = FAIR;
            break;
        }
    }
    for (int i = 0; i < list_size; i++)
        board[x_list[i]][y_list[i]] = 0;
    // delete[] top_sim;
    return result;
}

void MCT::propagate(Node *node, Result result)
{
    Player player = node->player;
    Node *cur = node;
    if (result == FAIR) // 平局：不更新胜负次数，只更新总次数（也可以有其他处理方式）
    {
        cur->tot_cnt++;
        for (cur = node; cur->parent; cur = back(cur))
        {
            cur->parent->tot_cnt++;
            cur->UCB = UCB((double)cur->win_cnt / (double)cur->tot_cnt, C, bonus(cur->tot_cnt, cur->parent->tot_cnt));
        }
    }
    else
    {
        cur->tot_cnt++;
        if ((player == cur->player) == (result == WIN))
            cur->win_cnt++;
        for (cur = node; cur->parent; cur = back(cur))
        {
            cur->parent->tot_cnt++;
            cur->UCB = UCB((double)cur->win_cnt / (double)cur->tot_cnt, C, bonus(cur->tot_cnt, cur->parent->tot_cnt));
            if ((player == cur->parent->player) == (result == WIN))
                cur->parent->win_cnt++;
        }
    }
}

MCT::MCT(int _M, int _N, int **_board, int *_top, int _lastX, int _lastY, int _noX, int _noY)
    : M(_M), N(_N), board(_board), top(_top), noX(_noX), noY(_noY), root(new Node(USER, _lastX, _lastY, nullptr)), rng(std::random_device{}())
{
    if (del_list.size())
    {
        for (auto node : del_list)
            delete node;
        del_list.clear();
    }
    del_list.push_back(root);
    // std::cout << "对手选点：" << _lastX << " " << _lastY << std::endl;
    root->state = EXPAND; // root 不做模拟，初始状态为待拓展
    get_children(root);
}

void MCT::run(int times)
{
    for (int i = 0; i < times; i++)
    {
        Node *node = select();
        switch (node->state)
        {
        case SIMULATE:
        {
            Result result = simulate(node);
            node->state = EXPAND;
            propagate(node, result);
            break;
        }
        case RESULT_FAIR:
        {
            propagate(node, FAIR);
            break;
        }
        case RESULT_WIN:
        {
            propagate(node, WIN);
            break;
        }
        case RESULT_LOSE:
        {
            propagate(node, LOSE);
            break;
        }
        default:
        {
            // std::cerr << "error: 选择了错误的节点" << std::endl;
            break;
        }
        }
    }
}

void MCT::get_action(int *x, int *y)
{
    if (!root->child_num) // 烂完了
    {
        for (int i = 0; i < N; i++)
            if (top[i])
            {
                *x = top[i] - 1;
                *y = i;
                return;
            }
    }
    if (root->state == RESULT_WIN || root->state == RESULT_LOSE || root->state == RESULT_FAIR)
    {
        Node *best = root->child_list[0];
        *x = best->lastX;
        *y = best->lastY;
    }
    else
    {
        Node *best = root->child_list[0];
        int max_win = best->tot_cnt;
        for (int i = 1; i < root->child_num; i++)
        {
            Node *child = root->child_list[i];
            int win = child->tot_cnt;
            if (win > max_win)
            {
                max_win = win;
                best = child;
            }
        }
        *x = best->lastX;
        *y = best->lastY;
    }
    // std::cout << "我的选点：" << *x << " " << *y << std::endl;
}