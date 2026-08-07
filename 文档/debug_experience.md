## 作用域：main() 的局部对象不会自动传进函数
即使你在 main() 里创建了：

AOFLog aoflog("append.aof");

另一个函数也看不到它。
void handle_read(...)
{
    aoflog.append(...);
}

## std::optional<std::string> 不是 std::string

你的：auto line = conn.pop_command_line();

返回的是：std::optional<std::string>

它表达：可能有一条完整命令 也可能暂时没有完整命令

## 字符串不是输入流

你尝试过：std::getline(contents, line);

但 contents 是：std::string

getline() 第一个参数要求的是输入流：std::istream&

所以需要：std::istringstream input(contents);

##  系统调用需要真实缓冲区

你最初把 ifstream 或空字符串当成 read() 的缓冲区，这是接口层次混淆。

read() 的契约是：read(fd, 可写内存地址, 最大字节数);

典型写法：char buffer[4096];
ssize_t n = read(fd_, buffer, sizeof(buffer));

然后：contents.append(buffer, static_cast<std::size_t>(n));

## 文件描述符不仅有“是否有效”，还有“访问模式”

你遇到：read: Bad file descriptor

但同一个 fd 的 write() 又能成功。

原因是打开方式为：O_WRONLY

它不是无效 fd，而是：对写操作有效，对读操作无效。

改为：O_RDWR | O_CREAT | O_APPEND

才能用同一个 fd 同时 replay 和 append。

## 7. 文件偏移决定下一次从哪里读写

同一个 fd 内部带有当前文件偏移。

replay 前需要：lseek(fd_, 0, SEEK_SET);

表示从文件开头读。

读完后可以：lseek(fd_, 0, SEEK_END);回到末尾。

而 O_APPEND 进一步保证每次 write() 实际落在末尾。

## 日志要放在边界上
最有价值的日志通常不是随便打印，而是放在模块边界和系统调用附近.

## perror() 只能解释当前 errno

你写过：
if (!aoflog.append(...)) {
    perror("append failed");
}

这不总是可靠，因为 append() 是你自己的函数，它返回 false 时，未必保证 errno 仍然对应错误。