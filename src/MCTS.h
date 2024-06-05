#ifndef MCTS_H
#define MCTS_H

#include <random>
#define MAX_BOARD_SIZE 12

enum State
{
    SIMULATE,    // 准备模拟
    EXPAND,      // 准备拓展
    SELECT,      // 准备选择
    RESULT_WIN,  // 已经赢了
    RESULT_FAIR, // 已经平局了
    RESULT_LOSE, // 已经输了

};

enum Player
{
    USER,
    MACHINE,
};

enum Result
{
    FAIR,
    LOSE,
    WIN
};

struct Node
{
    State state;
    Player player;
    int lastX, lastY;
    int win_cnt, tot_cnt;
    Node *parent;
    int child_num;
    Node *child_list[MAX_BOARD_SIZE];
    int expand_cnt;
    double UCB;
    Node(Player _player, int _lastX, int _lastY, Node *_parent)
        : state(SIMULATE), player(_player), lastX(_lastX), lastY(_lastY), win_cnt(0), tot_cnt(0), parent(_parent), child_num(0), expand_cnt(0), UCB(0) {}
};

class MCT
{
    int M, N;
    int **board;
    int *top;
    int noX, noY;
    Node *root;
    std::mt19937 rng;

private:
    inline int get_random(int l, int r) // 生成[l, r]之间的随机数
    {
        std::uniform_int_distribution<int> dist(l, r);
        return dist(rng);
    }
    inline Node *step(Node *node, int r); // 走一步(移动指针到子节点 + 更新棋盘)
    inline Node *back(Node *node);        // 回溯（移动指针到父节点 + 恢复棋盘）
    inline void get_children(Node *node); // 获取当前局面的所有合法子节点
    inline int best_child(Node *node);    // 选择最优的子节点

private:
    Node *select();                            // 按照UCB公式选择最优的子节点
    Result simulate(Node *node);               // 从当前局面开始模拟一局游戏，返回胜负结果
    void propagate(Node *node, Result result); // 回溯更新所有父节点的胜负次数

public:
    MCT(int _M, int _N, int **_board, int *_top, int _lastX, int _lastY, int _noX, int _noY);
    ~MCT() {};
    void run(int times);             // 运行一次 MCTS 算法
    void get_action(int *x, int *y); // 返回最优的下棋位置
};

#endif