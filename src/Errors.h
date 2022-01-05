// This header-only include defines a helpful set
//   of logging functions for the CCLI.

#ifndef CORTH_ERRORS_H
#define CORTH_ERRORS_H

namespace Corth {
	void DoLog(std::string msg, std::string prefix = "[LOG]", std::string suffix = "\n") {
        printf((prefix + ": %s" + suffix).c_str(), msg.c_str());
    }

    void DoLog(std::string msg, size_t line_num, std::string prefix = "[LOG]", std::string suffix = "\n") {
        printf((prefix + " LINE %zu: %s" + suffix).c_str(), line_num, msg.c_str());
    }

    void DoLog(std::string msg, size_t line_num, size_t column_num, std::string prefix = "[LOG]", std::string suffix = "\n") {
        printf((prefix + " LINE %zu, COL %zu: %s" + suffix).c_str(), line_num, column_num, msg.c_str());
    }

    void Error(std::string msg) {
        DoLog(msg, "\n[ERR]");
    }

    void Error(std::string msg, size_t line_num) {
        DoLog(msg, line_num, "\n[ERR]");
    }

    void Error(std::string msg, size_t line_num, size_t column_num) {
        DoLog(msg, line_num, column_num, "\n[ERR]");
    }

    void Error(std::string msg, std::exception e) {
        DoLog(msg + " (" + e.what() + ")", "\n[ERR]");
    }

    void Warning(std::string msg) {
        DoLog(msg, "[WRN]");
    }

    void Warning(std::string msg, size_t line_num) {
        DoLog(msg, line_num, "[WRN]");
    }

    void Warning(std::string msg, size_t line_num, size_t column_num) {
        DoLog(msg, line_num, column_num, "[WRN]");
    }

    void DbgLog(std::string msg) {
        DoLog(msg, "[DBG]");
    }

    void DbgLog(std::string msg, size_t line_num) {
        DoLog(msg, line_num, "[DBG]");
    }

    void DbgLog(std::string msg, size_t line_num, size_t column_num) {
        DoLog(msg, line_num, column_num, "[DBG]");
    }

    void Log(std::string msg) {
        DoLog(msg);
    }

    void Log(std::string msg, size_t line_num) {
        DoLog(msg, line_num);
    }

    void Log(std::string msg, size_t line_num, size_t column_num) {
        DoLog(msg, line_num, column_num);
    }

	void StackError() {
        Error("Stack protection invoked! (Did you forget to put the operator after the operands (i.e. `5 5 +` not `5 + 5`))?");
    }

    void StackError(size_t line_num) {
        Error("Stack protection invoked! (Did you forget to put the operator after the operands (i.e. `5 5 +` not `5 + 5`))?", line_num);
    }

    void StackError(size_t line_num, size_t column_num) {
        Error("Stack protection invoked! (Did you forget to put the operator after the operands (i.e. `5 5 +` not `5 + 5`))?", line_num, column_num);
    }
}
#endif
