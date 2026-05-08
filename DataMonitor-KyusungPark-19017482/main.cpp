// DataMonitor - 반도체 시료 생산주문관리 콘솔 모니터링 도구
// PoC: JSON 파일 기반 실시간 데이터 조회 관리자 도구

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

// ─────────────────────────────────────────────
//  Minimal JSON Parser (dependency-free)
// ─────────────────────────────────────────────

struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool   boolVal  = false;
    double numVal   = 0.0;
    std::string strVal;
    std::vector<JsonValue> arrayVal;
    std::map<std::string, JsonValue> objectVal;

    bool isNull()   const { return type == Type::Null;   }
    bool isNumber() const { return type == Type::Number; }
    bool isString() const { return type == Type::String; }
    bool isArray()  const { return type == Type::Array;  }

    double      getNumber() const { return numVal; }
    int         getInt()    const { return static_cast<int>(numVal); }
    const std::string& getString() const { return strVal; }
    const std::vector<JsonValue>& getArray() const { return arrayVal; }

    const JsonValue& operator[](const std::string& key) const {
        static JsonValue null;
        auto it = objectVal.find(key);
        return it == objectVal.end() ? null : it->second;
    }
};

class JsonParser {
    const std::string& src;
    size_t pos = 0;

    void skip() { while (pos < src.size() && std::isspace((unsigned char)src[pos])) pos++; }

    JsonValue parseValue() {
        skip();
        if (pos >= src.size()) throw std::runtime_error("JSON: unexpected end");
        char c = src[pos];
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return parseString();
        if (c == 't' || c == 'f') return parseBool();
        if (c == 'n') { pos += 4; return JsonValue{}; }
        if (c == '-' || std::isdigit((unsigned char)c)) return parseNumber();
        throw std::runtime_error(std::string("JSON: unexpected char '") + c + "'");
    }

    JsonValue parseObject() {
        JsonValue v; v.type = JsonValue::Type::Object;
        pos++; skip();
        if (src[pos] == '}') { pos++; return v; }
        while (true) {
            skip();
            auto key = parseString().strVal;
            skip(); pos++; // ':'
            v.objectVal[key] = parseValue();
            skip();
            if (src[pos] == '}') { pos++; break; }
            if (src[pos] == ',') { pos++; continue; }
            throw std::runtime_error("JSON: expected ',' or '}'");
        }
        return v;
    }

    JsonValue parseArray() {
        JsonValue v; v.type = JsonValue::Type::Array;
        pos++; skip();
        if (src[pos] == ']') { pos++; return v; }
        while (true) {
            v.arrayVal.push_back(parseValue());
            skip();
            if (src[pos] == ']') { pos++; break; }
            if (src[pos] == ',') { pos++; continue; }
            throw std::runtime_error("JSON: expected ',' or ']'");
        }
        return v;
    }

    JsonValue parseString() {
        JsonValue v; v.type = JsonValue::Type::String;
        pos++; // skip "
        while (pos < src.size() && src[pos] != '"') {
            if (src[pos] == '\\') {
                pos++;
                switch (src[pos]) {
                    case '"':  v.strVal += '"';  break;
                    case '\\': v.strVal += '\\'; break;
                    case 'n':  v.strVal += '\n'; break;
                    case 'r':  v.strVal += '\r'; break;
                    case 't':  v.strVal += '\t'; break;
                    default:   v.strVal += src[pos]; break;
                }
            } else {
                v.strVal += src[pos];
            }
            pos++;
        }
        pos++; // skip "
        return v;
    }

    JsonValue parseBool() {
        JsonValue v; v.type = JsonValue::Type::Bool;
        if (src.substr(pos, 4) == "true")  { v.boolVal = true;  pos += 4; }
        else                               { v.boolVal = false; pos += 5; }
        return v;
    }

    JsonValue parseNumber() {
        size_t s = pos;
        if (src[pos] == '-') pos++;
        while (pos < src.size() && std::isdigit((unsigned char)src[pos])) pos++;
        if (pos < src.size() && src[pos] == '.') {
            pos++;
            while (pos < src.size() && std::isdigit((unsigned char)src[pos])) pos++;
        }
        JsonValue v; v.type = JsonValue::Type::Number;
        v.numVal = std::stod(src.substr(s, pos - s));
        return v;
    }

public:
    explicit JsonParser(const std::string& s) : src(s) {}
    JsonValue parse() { return parseValue(); }
};

// ─────────────────────────────────────────────
//  Domain Model: 반도체 생산 주문
// ─────────────────────────────────────────────

struct Order {
    std::string orderId;
    std::string sampleType;   // 시료 종류
    std::string grade;         // 등급
    int         quantity  = 0;
    std::string unit;
    std::string status;        // 대기중 | 진행중 | 완료 | 취소
    std::string priority;      // HIGH | MEDIUM | LOW
    std::string customer;      // 고객사
    std::string createdDate;
    std::string deadline;
    std::string processStep;   // 공정 단계
    std::string lotId;
    double      defectRate = 0.0;  // 불량률 (0.0 ~ 1.0)
    std::string operatorName;
    std::string notes;

    static Order fromJson(const JsonValue& j) {
        Order o;
        o.orderId      = j["order_id"].getString();
        o.sampleType   = j["sample_type"].getString();
        o.grade        = j["grade"].getString();
        o.quantity     = j["quantity"].getInt();
        o.unit         = j["unit"].getString();
        o.status       = j["status"].getString();
        o.priority     = j["priority"].getString();
        o.customer     = j["customer"].getString();
        o.createdDate  = j["created_date"].getString();
        o.deadline     = j["deadline"].getString();
        o.processStep  = j["process_step"].getString();
        o.lotId        = j["lot_id"].getString();
        o.defectRate   = j["defect_rate"].getNumber();
        o.operatorName = j["operator"].getString();
        o.notes        = j["notes"].getString();
        return o;
    }

    int priorityRank() const {
        if (priority == "HIGH")   return 0;
        if (priority == "MEDIUM") return 1;
        return 2;
    }
};

// ─────────────────────────────────────────────
//  Console Utilities
// ─────────────────────────────────────────────

namespace con {
    const std::string RST = "\033[0m";
    const std::string BLD = "\033[1m";
    const std::string RED = "\033[31m";
    const std::string GRN = "\033[32m";
    const std::string YLW = "\033[33m";
    const std::string BLU = "\033[34m";
    const std::string CYN = "\033[36m";
    const std::string WHT = "\033[37m";

    void enableAnsi() {
#ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD  m = 0;
        GetConsoleMode(h, &m);
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        SetConsoleOutputCP(CP_UTF8);
#endif
    }

    void clear() {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }

    void sep(char c = '-', int w = 82) { std::cout << std::string(w, c) << "\n"; }

    void header(const std::string& title) {
        sep('=');
        std::cout << BLD << BLU
            << "  [DataMonitor] 반도체 시료 생산주문관리 :: " << title
            << RST << "\n";
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        char buf[32];
        struct tm tm_info {};
        localtime_s(&tm_info, &t);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
        std::cout << "  조회 시각: " << buf << "\n";
        sep('=');
    }

    std::string colorStatus(const std::string& s) {
        if (s == "진행중") return CYN  + s + RST;
        if (s == "완료")   return GRN  + s + RST;
        if (s == "대기중") return YLW  + s + RST;
        if (s == "취소")   return RED  + s + RST;
        return s;
    }

    std::string colorPriority(const std::string& p) {
        if (p == "HIGH")   return RED + p + RST;
        if (p == "MEDIUM") return YLW + p + RST;
        if (p == "LOW")    return GRN + p + RST;
        return p;
    }

    void waitEnter() {
        std::cout << "\n\n  " << YLW << "[Enter]" << RST << " 를 눌러 메뉴로 돌아가기...";
        std::cin.ignore();
        std::cin.get();
    }
}

// ─────────────────────────────────────────────
//  DataMonitor Application
// ─────────────────────────────────────────────

class DataMonitor {
    std::string          dataPath;
    std::vector<Order>   orders;

    // JSON 파일 로드
    static std::string readFile(const std::string& path) {
        std::ifstream f(path);
        if (!f) throw std::runtime_error("파일 열기 실패: " + path);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    // 주문 테이블 출력
    void printTable(const std::vector<Order>& list) const {
        if (list.empty()) { std::cout << "  조회된 주문이 없습니다.\n"; return; }

        std::cout << con::BLD
            << std::left
            << std::setw(18) << "주문번호"
            << std::setw(18) << "시료종류"
            << std::setw(14) << "고객사"
            << std::setw(10) << "수량"
            << std::setw(8)  << "상태"
            << std::setw(8)  << "우선순위"
            << std::setw(12) << "마감일"
            << con::RST << "\n";
        con::sep();

        for (const auto& o : list) {
            // ANSI 색상 코드가 setw 너비 계산에 포함되므로 색상 문자열은 별도 출력
            std::cout << std::left
                << std::setw(18) << o.orderId
                << std::setw(18) << o.sampleType
                << std::setw(14) << o.customer
                << std::setw(10) << (std::to_string(o.quantity) + " " + o.unit);
            // 색상 적용 항목은 패딩 직접 처리
            auto st = con::colorStatus(o.status);
            auto pr = con::colorPriority(o.priority);
            std::cout << st << std::string(o.status.size() < 4 ? 16 : 14, ' ')
                      << pr << std::string(8 - o.priority.size(), ' ')
                      << o.deadline << "\n";
        }
    }

    // 메뉴 1: 전체 목록
    void showAll() const {
        con::header("전체 주문 목록");
        std::cout << "  총 " << orders.size() << "건\n\n";
        printTable(orders);
    }

    // 메뉴 2: 상태별 필터
    void filterByStatus() const {
        con::header("상태별 조회");
        std::cout << "  1. 대기중\n  2. 진행중\n  3. 완료\n  4. 취소\n";
        con::sep();
        std::cout << "선택 > ";
        int ch; std::cin >> ch;
        const std::string targets[] = {"", "대기중", "진행중", "완료", "취소"};
        if (ch < 1 || ch > 4) { std::cout << "  잘못된 선택.\n"; return; }

        std::vector<Order> out;
        for (const auto& o : orders)
            if (o.status == targets[ch]) out.push_back(o);

        con::clear(); con::header("상태별: " + targets[ch]);
        std::cout << "  " << out.size() << "건\n\n";
        printTable(out);
    }

    // 메뉴 3: 시료종류별 필터
    void filterBySampleType() const {
        std::vector<std::string> types;
        for (const auto& o : orders)
            if (std::find(types.begin(), types.end(), o.sampleType) == types.end())
                types.push_back(o.sampleType);

        con::header("시료종류별 조회");
        for (size_t i = 0; i < types.size(); i++)
            std::cout << "  " << (i + 1) << ". " << types[i] << "\n";
        con::sep();
        std::cout << "선택 > ";
        int ch; std::cin >> ch;
        if (ch < 1 || ch > static_cast<int>(types.size())) { std::cout << "  잘못된 선택.\n"; return; }

        const std::string& target = types[ch - 1];
        std::vector<Order> out;
        for (const auto& o : orders)
            if (o.sampleType == target) out.push_back(o);

        con::clear(); con::header("시료종류: " + target);
        std::cout << "  " << out.size() << "건\n\n";
        printTable(out);
    }

    // 메뉴 4: 우선순위 정렬
    void sortByPriority() const {
        con::header("우선순위별 정렬 (HIGH → MEDIUM → LOW)");
        auto sorted = orders;
        std::sort(sorted.begin(), sorted.end(),
            [](const Order& a, const Order& b) { return a.priorityRank() < b.priorityRank(); });
        std::cout << "  총 " << sorted.size() << "건\n\n";
        printTable(sorted);
    }

    // 메뉴 5: 상세 조회
    void showDetail() const {
        con::header("주문 상세 조회");
        std::cout << "주문번호 입력 > ";
        std::string id; std::cin >> id;

        for (const auto& o : orders) {
            if (o.orderId == id) {
                con::clear(); con::header("주문 상세: " + o.orderId);
                con::sep();
                auto row = [](const std::string& label, const std::string& val) {
                    std::cout << "  " << con::BLD << std::setw(12) << label << con::RST << ": " << val << "\n";
                };
                row("주문번호",  o.orderId);
                row("LOT번호",   o.lotId);
                row("시료종류",  o.sampleType + " (" + o.grade + " Grade)");
                row("수량",      std::to_string(o.quantity) + " " + o.unit);
                row("고객사",    o.customer);
                row("상태",      con::colorStatus(o.status));
                row("우선순위",  con::colorPriority(o.priority));
                row("공정단계",  o.processStep);
                row("담당자",    o.operatorName);
                row("불량률",    [&]() {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(2) << o.defectRate * 100.0 << "%";
                    return ss.str();
                }());
                row("등록일",    o.createdDate);
                row("마감일",    o.deadline);
                row("비고",      o.notes);
                con::sep();
                return;
            }
        }
        std::cout << "  주문번호 '" << id << "' 를 찾을 수 없습니다.\n";
    }

    // 메뉴 6: 통계
    void showStats() const {
        con::header("주문 통계 요약");

        std::map<std::string, int> bySt, byPr, byTy;
        double sumDefect = 0.0;
        int    sumQty    = 0;
        for (const auto& o : orders) {
            bySt[o.status]++;
            byPr[o.priority]++;
            byTy[o.sampleType]++;
            sumDefect += o.defectRate;
            sumQty    += o.quantity;
        }

        auto section = [](const std::string& title) {
            std::cout << "\n" << con::BLD << "  [" << title << "]\n" << con::RST;
        };

        section("상태별 현황");
        for (const auto& [k, v] : bySt)
            std::cout << "    " << std::left << std::setw(8) << k << ": " << v << "건\n";

        section("우선순위별 현황");
        for (const auto& [k, v] : byPr)
            std::cout << "    " << std::left << std::setw(8) << k << ": " << v << "건\n";

        section("시료종류별 현황");
        for (const auto& [k, v] : byTy)
            std::cout << "    " << std::left << std::setw(22) << k << ": " << v << "건\n";

        section("생산 요약");
        double avgDefect = orders.empty() ? 0.0 : sumDefect / orders.size() * 100.0;
        std::cout << "    총 주문 건수  : " << orders.size() << "건\n";
        std::cout << "    총 생산 수량  : " << sumQty << "\n";
        std::cout << "    평균 불량률   : "
                  << std::fixed << std::setprecision(2) << avgDefect << "%\n";
        con::sep();
    }

    // 메뉴 7: 새로고침
    void refresh() {
        std::cout << "  데이터 새로고침 중...\n";
        load();
        std::cout << "  완료: " << con::GRN << orders.size() << "건" << con::RST << " 로드됨\n";
    }

public:
    explicit DataMonitor(const std::string& path) : dataPath(path) {}

    void load() {
        std::string json = readFile(dataPath);
        JsonParser  parser(json);
        JsonValue   root = parser.parse();

        orders.clear();
        for (const auto& item : root["orders"].getArray())
            orders.push_back(Order::fromJson(item));
    }

    void run() {
        con::enableAnsi();
        try {
            load();
        } catch (const std::exception& e) {
            std::cerr << "[오류] 초기 로드 실패: " << e.what() << "\n";
            return;
        }

        while (true) {
            con::clear();
            con::header("메인 메뉴");
            std::cout << "\n"
                << "  1. 전체 주문 목록 조회\n"
                << "  2. 상태별 필터링\n"
                << "  3. 시료종류별 필터링\n"
                << "  4. 우선순위별 정렬\n"
                << "  5. 주문 상세 조회\n"
                << "  6. 통계 요약\n"
                << "  7. 데이터 새로고침\n"
                << "  0. 종료\n\n";
            con::sep();
            std::cout << "선택 > ";

            int choice = -1;
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(4096, '\n');
                continue;
            }

            if (choice == 0) {
                std::cout << "\n  DataMonitor를 종료합니다.\n\n";
                break;
            }

            con::clear();
            switch (choice) {
                case 1: showAll();           break;
                case 2: filterByStatus();    break;
                case 3: filterBySampleType();break;
                case 4: sortByPriority();    break;
                case 5: showDetail();        break;
                case 6: showStats();         break;
                case 7: refresh();           break;
                default: std::cout << "  잘못된 선택입니다.\n"; break;
            }
            con::waitEnter();
        }
    }
};

// ─────────────────────────────────────────────
//  Entry Point
// ─────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "data/orders.json";
    DataMonitor monitor(path);
    monitor.run();
    return 0;
}
