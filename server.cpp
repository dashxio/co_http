#include "http_server.h"
#include "fmt/format.h"

void server() {
    io_context ctx;
    HttpServer::makeServerWithListenPort("127.0.0.1", "8080")->start();
    ctx.join();
}

int main() {

#if _WIN32 // 热知识：64 位 Windows 也会定义 _WIN32 宏，所以 _WIN32 可以用于检测是否是 Windows 系统
    setlocale(LC_ALL, ".utf-8");  // 设置标准库调用系统 API 所用的编码，用于 fopen，ifstream 等函数，这样就可以用标准库函数打印了
    SetConsoleOutputCP(CP_UTF8); // 设置控制台输出编码，或者写 system("chcp 65001") 也行，这里 CP_UTF8 = 65001
    SetConsoleCP(CP_UTF8); // 设置控制台输入编码，用于 std::cin
#endif

    try {
        server();
    }
    catch (std::exception const & e) {
        fmt::println("错误: {}", e.what()); //这里面调的w系的接口去打印，所以无需setlocale()也能直接打印utf8字符
        //std::cout << e.what() << "\n"; //这里面调的a系接口去打印，所以需要setlocale(LC_ALL, ".utf-8")才能打印utf8字符
    }

    return 0;
}
