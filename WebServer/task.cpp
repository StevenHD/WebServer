/*
 * @author: hlhd
 */

#include "task.h"

/* 定义http响应的一些状态信息 */
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";

//网站根目录，文件夹内存放请求的资源和跳转的html文件
const char* doc_root = "/home/hlhd/CLionProjects/WebServer";

int Task::_usr_cnt = 0;
int Task::_task_epollfd = -1;

/* 关闭http连接 */
void Task::close_conn(bool read_close)
{
    // 关闭一个连接，客户数量减一
    if (read_close &&(_task_socketfd != -1))
    {
        printf("close %d\n", _task_socketfd);
        remove_fd(_task_epollfd, _task_socketfd);
        _task_socketfd = -1;
        _usr_cnt --;
    }
}

/* 初始化套接字地址和http连接 */
void Task::init(int sockfd, const sockaddr_in &addr, char *root)
{
    _task_socketfd = sockfd;
    _task_address = addr;

    add_fd(_task_epollfd, _task_socketfd, true);
    _usr_cnt ++;

    // 当浏览器出现连接重置时
    // 可能是网站根目录出错或http响应格式出错或者访问的文件中内容完全为空
    doc_root = root;
    init_conn();
}

/* 初始化新接受的连接 */
void Task::init_conn()
{
    bytes_to_send = 0;
    bytes_have_send = 0;

    // check_state默认为分析请求行状态
    _check_state = CHECK_STATE_REQUESTLINE;

    _linker = false;
    _method = GET;
    _url = 0;
    _version = 0;
    _content_len = 0;
    _host = 0;
    _start_line = 0;
    _checked_idx = 0;
    _read_idx = 0;
    _write_idx = 0;
    cgi = 0;
    _task_state = 0;

    memset(_read_buf, '\0', READ_BUFFER_SIZE);
    memset(_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(_real_filename, '\0', FILENAME_LEN);
}

/* [从状态机], 用于分析出一行内容 */
Task::LINE_STATUS Task::parse_line()
{
    /* 从状态机读取一行，分析是请求报文的哪一部分 */
    // 返回值为行的读取状态，有LINE_OK,LINE_BAD,LINE_OPEN

    // _read_idx指向缓冲区m_read_buf的数据末尾的下一个字节
    // _checked_idx指向[从状态机]当前正在分析的字节
    char temp;
    for (; _checked_idx < _read_idx; _checked_idx ++ )
    {
        /* temp为将要分析的字节 */
        temp = _read_buf[_checked_idx];

        /* 如果当前是\r字符，则有可能会读取到完整行 */
        if (temp == '\r')
        {
            if ((_checked_idx + 1) == _read_idx)
                return LINE_OPEN;
            else if (_read_buf[_checked_idx + 1] == '\n')
            {
                _read_buf[_checked_idx ++] = '\0';
                _read_buf[_checked_idx ++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
            /* 如果当前字符是\n，也有可能读取到完整行 */
        else if (temp == '\n')
        {
            if (_checked_idx > 1 &&
                _read_buf[_checked_idx - 1] == '\r')
            {
                _read_buf[_checked_idx - 1] = '\0';
                _read_buf[_checked_idx ++] = '\0';

                return LINE_OK;
            }

            return LINE_BAD;
        }
    }

    /* 并没有找到\r\n，需要继续接收 */
    return LINE_OPEN;
}

/* 循环读取浏览器端发来的全部数据，直到无数据可读或对方关闭连接 */
bool Task::read_data()
{
    // 非阻塞ET工作模式下，需要一次性将数据读完
    if (_read_idx >= READ_BUFFER_SIZE) return false;
    int bytes_read = 0;

    // ET读数据
    while (true)
    {
        /* 从套接字接收数据，存储在_task_read_buf缓冲区 */
        bytes_read = recv(_task_socketfd,
                          _read_buf + _read_idx,
                          READ_BUFFER_SIZE - _read_idx,
                          0);
        if (bytes_read == -1)
        {
            /* 非阻塞ET模式下，需要一次性将数据读完 */
            if (errno == EAGAIN) break;
            return false;
        }
        else if (bytes_read == 0)
        {
            return false;
        }

        /* 修改_read_idx的读取字节数 */
        _read_idx += bytes_read;
    }

    return true;
}

/* 主状态机解析报文中的请求行数据 */
Task::HTTP_CODE Task::parse_request_line(char *text)
{
    // 解析http请求行，获得请求方法，目标url及http版本号
    _url = strpbrk(text, " \t");
    if (!_url) return BAD_REQUEST;

    *_url ++ = '\0';

    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        _method = GET;
    else if (strcasecmp(method, "POST") == 0)
    {
        _method = POST;
        cgi = 1;
    }
    else return BAD_REQUEST;

    /* _url跳过空格和\t字符，指向请求资源的第一个字符 */
    _url += strspn(_url, " \t");

    _version = strpbrk(_url, " \t");
    if (_version) return BAD_REQUEST;

    *_version ++ = '\0';
    _version += strspn(_version, " \t");

    /* 仅支持HTTP/1.1 */
    if (strcasecmp(_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;

    if (strncasecmp(_url, "http://", 7) == 0)
    {
        _url += 7;
        _url = strchr(_url, '/');
    }

    if (strncasecmp(_url, "https://", 8) == 0)
    {
        _url += 8;
        _url = strchr(_url, '/');
    }

    if (_url || _url[0] != '/')
        return BAD_REQUEST;

    if (strlen(_url) == 1)
        strcat(_url, "index.html");

    /* 请求行处理完毕，将主状态机转移处理请求头 */
    _check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

/* 主状态机解析报文中的请求头数据 */
Task::HTTP_CODE Task::parse_request_header(char *text)
{
    // 解析http请求的一个头部信息

    // 判断是空行还是请求头
    if (text[0] == '\0')
    {
        if (_content_len != 0)
        {
            _check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }

    // 解析请求头部连接字段
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;

        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            // 如果是长连接，则将_linker标志设置为true
            _linker = true;
        }
    }

    // 解析请求头部内容长度字段
    else if (strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        _content_len = atol(text);
    }

    // 解析请求头部HOST字段
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        _host = text;
    }
    else
    {
        printf("Unknow Header: %s\n", text);
    }

    return NO_REQUEST;
}

/* 主状态机解析报文中的请求内容 */
Task::HTTP_CODE Task::parse_request_content(char *text)
{
    // 判断http请求是否被完整读入
    // 判断buffer中是否读取了消息体
    if (_read_idx >= (_content_len + _checked_idx))
    {
        text[_content_len] = '\0';
        _header_data = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

/* 从_task_read_buf读取，并处理请求报文 */
Task::HTTP_CODE Task::process_read()
{
    /* 初始化[从状态机]的状态和HTTP请求解析结果 */
    LINE_STATUS _lineStatus = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = nullptr;

    /* parse_line为[从状态机]的具体实现 */
    while ((_check_state == CHECK_STATE_CONTENT && _lineStatus == LINE_OK) ||
           (_lineStatus = parse_line()) == LINE_OK)
    {
        text = get_line();

        /* _start_line是每一个数据行在_read_buf中的起始位置 */
        /* _checked_idx表示[从状态机]在_read_buf中读取的位置 */
        _start_line = _checked_idx;

        /* 主状态机的三种状态转移逻辑 */
        switch (_check_state) {
            case CHECK_STATE_REQUESTLINE:
            {
                /* 解析请求行 */
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }

            case CHECK_STATE_HEADER:
            {
                /* 解析请求头 */
                ret = parse_request_header(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;

                    /* 完整解析GET请求后，跳转到报文响应函数 */
                else if (ret == GET_REQUEST)
                    return do_request();
                break;
            }

            case CHECK_STATE_CONTENT:
            {
                /* 解析消息体 */
                ret = parse_request_content(text);

                /* 完整解析POST请求后，跳转到报文响应函数 */
                if (ret == GET_REQUEST)
                    return do_request();

                /* 解析完消息体即完成报文解析
                 * 避免再次进入循环，更新line_status */
                _lineStatus = LINE_OPEN;
                break;
            }

            default:
                return INTERNAL_ERROR;
        }
    }

    return NO_REQUEST;
}

/* 生成响应报文 */
Task::HTTP_CODE Task::do_request()
{
    // 将初始化的_real_file赋值为网站根目录
    strcpy(_real_filename, doc_root);
    int len = strlen(doc_root);

    // 找到_url中/的位置
    const char *p = strrchr(_url, '/');

    // 直接将url与网站目录拼接，请求服务器上的一个图片
    strncpy(_real_filename, _url, FILENAME_LEN - len - 1);

    // 通过stat获取请求资源文件信息，成功则将信息更新到_file_stat结构体
    // 失败返回NO_RESOURCE状态，表示资源不存在
    if (stat(_real_filename, &_file_stat) < 0)
        return NO_RESOURCE;

    // 判断文件的权限，是否可读，不可读则返回FORBIDDEN_REQUEST状态
    if (!(_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    // 判断文件类型，如果是目录，则返回BAD_REQUEST，表示请求报文有误
    if (S_ISDIR(_file_stat.st_mode))
        return BAD_REQUEST;

    // 以只读方式获取文件描述符，通过mmap将该文件映射到内存中
    int fd = open(_real_filename, O_RDONLY);
    _file_addr = (char*)mmap(0, _file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);
    return FILE_REQUEST;    // 表示请求文件存在，且可以访问
}

void Task::unmap()
{
    if (_file_addr)
    {
        munmap(_file_addr, _file_stat.st_size);
        _file_addr = 0;
    }
}

/* 响应报文写入函数 */
bool Task::write_data()
{
    int ret = 0;
    int newadd = 0;

    // 要发送的数据长度为0
    // 表示响应报文为空，一般不会出现这种情况
    if (bytes_to_send = 0)
    {
        reset_oneshot(_task_epollfd,
                      _task_socketfd,
                      EPOLLIN);
        init_conn();
        return true;
    }

    while (1)
    {
        ret = writev(_task_socketfd, _IOV, _IOV_count);

        if (ret > 0)
        {
            bytes_have_send += ret;
            newadd = bytes_have_send - _write_idx;
        }

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                if (bytes_have_send >= _IOV[0].iov_len)
                {
                    // 不再继续发送头部信息
                    _IOV[0].iov_len = 0;
                    _IOV[1].iov_base = _file_addr + newadd;
                    _IOV[1].iov_len = bytes_have_send;
                }
                else
                {
                    _IOV[0].iov_base = _write_buf + bytes_to_send;
                    _IOV[0].iov_len = _IOV[0].iov_len - bytes_have_send;
                }

                // 重新注册写事件
                reset_oneshot(_task_epollfd,
                              _task_socketfd,
                              EPOLLOUT);
                return true;
            }

            // 如果发送失败，但不是缓冲区问题，取消映射
            unmap();
            return false;
        }

        bytes_to_send -= ret;
        if (bytes_to_send <= 0)
        {
            unmap();
            reset_oneshot(_task_epollfd, _task_socketfd,
                          EPOLLIN);

            if (_linker)
            {
                // 长连接
                init_conn();
                return true;
            }
            else
            {
                // 短连接
                return false;
            }
        }
    }
}

bool Task::add_response(const char* format, ...)
{
    /* 如果写入内容超出_write_buf大小则报错 */
    if (_write_idx >= WRITE_BUFFER_SIZE) return false;

    /* 定义可变参数列表 */
    va_list arg_list;

    /* 将变量arg_list初始化为传入参数 */
    va_start(arg_list, format);

    /* 将数据format从可变参数列表写入缓冲区，返回写入数据的长度 */
    int len = vsnprintf(_write_buf + _write_idx,
                        WRITE_BUFFER_SIZE - 1 - _write_idx,
                        format,
                        arg_list);

    /* 如果写入的数据长度超过缓冲区剩余空间，则报错 */
    if (len >= (WRITE_BUFFER_SIZE - 1 - _write_idx))
    {
        va_end(arg_list);
        return false;
    }

    /* 更新_write_idx位置 */
    _write_idx += len;

    /* 清空可变参列表 */
    va_end(arg_list);

    return true;
}

/* 添加状态行 */
bool Task::add_status_line(int status,const char* title)
{
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

/* 添加消息报头，具体的添加文本长度、连接状态和空行 */
bool Task::add_headers(int content_len)
{
    add_content_length(content_len);
    add_linker();
    add_blank_line();
}

/* 添加Content-Length，表示响应报文的长度 */
bool Task::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n", content_len);
}

bool Task::add_content_type()
{
    return add_response("Content-Type:%s\r\n", "text/html");
}

/* 添加连接状态，通知浏览器端是保持连接还是关闭 */
bool Task::add_linker()
{
    return add_response("Connection:%s\r\n",
                        (_linker == true) ? "keep-alive" : "close");
}

/* 添加空行 */
bool Task::add_blank_line()
{
    return add_response("%s", "\r\n");
}

/* 添加文本content */
bool Task::add_content(const char* content)
{
    return add_response("%s", content);
}

/* 向_task_write_buf写入响应报文数据 */
bool Task::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
        case INTERNAL_ERROR:
        {
            add_status_line(500, error_500_title);
            add_headers(strlen(error_500_form));
            if (!add_content(error_500_form)) return false;

            break;
        }
        case BAD_REQUEST:
        {
            add_status_line(404, error_404_title);
            add_headers(strlen(error_404_form));
            if (!add_content(error_404_form)) return false;

            break;
        }
        case FORBIDDEN_REQUEST:
        {
            add_status_line(403, error_403_title);
            add_headers(strlen(error_403_form));
            if (!add_content(error_403_form)) return false;

            break;
        }
        case FILE_REQUEST:
        {
            add_status_line(200, ok_200_title);
            if (_file_stat.st_size != 0)
            {
                add_headers(_file_stat.st_size);
                _IOV[0].iov_base = _write_buf;
                _IOV[0].iov_len = _write_idx;
                _IOV[1].iov_base = _file_addr;
                _IOV[1].iov_len = _file_stat.st_size;
                _IOV_count = 2;
                bytes_to_send = _write_idx + _file_stat.st_size;
                return true;
            }
            else
            {
                const char *ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if (!add_content(ok_string)) return false;
            }
        }
        default:
            return false;
    }

    _IOV[0].iov_base = _write_buf;
    _IOV[0].iov_len = _write_idx;
    _IOV_count = 1;

    bytes_to_send = _write_idx;

    return true;
}

void Task::process()
{
    HTTP_CODE read_ret = process_read();

    /* NO_REQUEST，表示请求不完整，需要继续接收请求数据 */
    if (read_ret == NO_REQUEST)
    {
        /* 注册并监听读事件 */
        reset_oneshot(_task_epollfd, _task_socketfd, EPOLLIN);
        return;
    }

    /* 调用process_write完成报文响应 */
    bool write_ret = process_write(read_ret);
    if (!write_ret) close_conn(true);

    /* 注册并监听写事件 */
    reset_oneshot(_task_epollfd, _task_socketfd, EPOLLOUT);
}
