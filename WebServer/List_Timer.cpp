#include "List_Timer.h"

List_Timer::List_Timer() : _prev(nullptr), _next(nullptr) {}

Sort_Timer_List::Sort_Timer_List() : _head(nullptr), _tail(nullptr) {}

/* 链表被销毁的时候，删除其中所有的定时器 */
Sort_Timer_List::~Sort_Timer_List()
{
    List_Timer* tmp = _head;
    while (tmp)
    {
        _head = tmp->_next;
        delete tmp;
        tmp = _head;
    }
}

/* 将目标定时器timer添加在链表当中 */
void Sort_Timer_List::add_timer(List_Timer *timer)
{
    if (!timer) return;
    if (!_head)
    {
        _head = _tail = timer;
        return;
    }

    /*
     * 如果目标定时器timer的超时时间小于
     * 当前链表中所有定时器的超时时间，
     * 则把该定时器插入链表头部，作为链表新的头节点
     * */
    if (timer->_expireTime < _head->_expireTime)
    {
        timer->_next = _head;
        _head->_prev = timer;
        _head = timer;
        return;
    }
    else add_timer(timer, _head);
}

/* 一个重载的辅助函数
 * 它被公有的add_timer函数和adjust_timer函数调用
 * 该函数表示将目标定时器timer添加到节点List_head之后的部分链表中
 */
void Sort_Timer_List::add_timer(List_Timer *timer, List_Timer *List_head)
{
    List_Timer* tmp = List_head;
    List_Timer* cur = tmp->_next;

    /* 遍历List_head节点之后的部分链表
     * 直到找到一个超时时间大于目标定时器timer超时时间的节点
     * 并将目标定时器timer插入到该节点之前 */
    while (cur)
    {
        if (timer->_expireTime < cur->_expireTime)
        {
            tmp->_next = timer;
            timer->_next = cur;
            cur->_prev = timer;
            timer->_prev = tmp;
            break;
        }
        tmp = cur;
        cur = cur->_next;
    }

    /* 如果遍历完List_head节点之后的部分链表
     * 依然没找到超时时间大于目标定时器超时时间的节点
     * 就将目标定时器插入链表尾部，并把它设置成链表新的尾节点 */
    if (!cur)
    {
        tmp->_next = timer;
        timer->_prev = tmp;
        timer->_next = nullptr;
        _tail = timer;
    }
}

/*
 * 当某个定时任务发生变化的时候
 * 调整对应的定时器在链表中的位置
 * 这个函数只考虑【被调整的定时器的超时时间延长】的情况
 * 也就是说【该定时器】需要往链表的尾部移动
 */
void Sort_Timer_List::adjust_timer(List_Timer *timer)
{
    if (!timer) return;
    List_Timer* tmp = timer->_next;

    /*
     * 如果被调整的目标定时器处在链表尾部
     * 或者该定时器新的超时值仍然小于其下一个定时器的超时值
     * 则不用调整
     */
    if (!tmp || (timer->_expireTime < tmp->_expireTime))
    {
        return;
    }

    /*
     * 如果目标定时器是链表的头节点
     * 则将该定时器从链表中取出并重新插入链表
     */
    if (timer == _head)
    {
        _head = _head->_next;
        _head->_prev = nullptr;
        timer->_next = nullptr;
        add_timer(timer, _head);
    }
    /*
     * 如果目标定时器不是链表的头节点
     * 则将该定时器从链表中取出
     * 然后插入其原来所在位置之后的部分链表中
     */
    else
    {
        timer->_prev->_next = timer->_next;
        timer->_next->_prev = timer->_prev;
        add_timer(timer, timer->_next);
    }
}

/*
 * 将目标定时器timer从链表中删除
 */
void Sort_Timer_List::del_timer(List_Timer *timer)
{
    if (!timer) return;
    /*
     * 下面这个条件成立表示链表中只有一个定时器
     * 也就是目标定时器
     */
    if ((timer == _head) && (timer == _tail))
    {
        delete timer;
        _head = nullptr;
        _tail = nullptr;
        return;
    }

    /*
     * 如果链表中至少有2个定时器
     * 而且目标定时器是链表的头节点
     * 则将链表的头节点重置为原头节点的下一个节点
     * 然后删除目标定时器timer
     */
    if (timer == _head)
    {
        _head = _head->_next;
        _head->_prev = nullptr;
        delete timer;
        return;
    }
    /*
     * 如果链表中至少有2个定时器
     * 而且目标定时器是链表的尾节点
     * 则将链表的尾节点重置为原尾节点的前一个节点
     * 然后删除目标定时器timer
     */
    if (timer == _tail)
    {
        _tail = _tail->_prev;
        _tail->_next = nullptr;
        delete timer;
        return;
    }

    /*
     * 如果目标定时器位于链表的中间
     * 则把它前后的定时器串联起来
     * 然后删除目标定时器
     */
    timer->_prev->_next = timer->_next;
    timer->_next->_prev = timer->_prev;
    delete timer;
}

/*
 * SIGALRM信号每次被触发就在
 * 其信号处理函数（如果使用的是统一事件源，则是主函数）中
 * 执行一次tick函数
 * 从而可以处理链表上[到期的任务]
 */
void Sort_Timer_List::tick()
{
    if (!_head) return;
    printf("timer tick\n");

    /* 获得系统当前的时间 */
    time_t cur_time = time(nullptr);
    List_Timer* tmp = _head;

    /*
     * 从头节点开始依次处理每个定时器
     * 直到遇到一个尚未到期的定时器
     * 这就是定时器的核心逻辑
     */
    while (tmp)
    {
        /*
         * 因为每个定时器都使用绝对时间作为超时值
         * 所以我们可以把定时器的超时值和系统当前时间比较
         * 从而判断定时器是否到期
         */
        if (tmp->_expireTime > cur_time) break;

        /*
         * 调用定时器的回调函数
         * 从而执行定时任务
         */
        tmp->cb_func(tmp->_usrData);

        /*
         * 执行完定时器中的定时任务后
         * 就将它从链表中删除
         * 并且重置链表头结点
         */
        _head = tmp->_next;
        if (_head)
        {
            _head->_prev = nullptr;
        }
        delete tmp;
        tmp = _head;
    }
}