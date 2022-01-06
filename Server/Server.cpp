#include <iostream>
#include <iomanip>
#include <chrono>
#include <functional>
#include <winsock2.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stdio.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#pragma warning(disable: 4996)

SYSTEMTIME st;
int adrr1[13] = {310, 311, 361, 314, 324, 325, 366, 367, 365, 331, 332, 333, 270};
int adrr2[14] = {76, 101, 102, 103, 110, 120, 111, 121, 113, 150, 140, 165, 260, 273};

//ARINC-429

#pragma pack(push, 1)
struct BNR //структура для двоичного числа
{
    unsigned int address : 8; //адрес
    unsigned int data : 20; //данные
    unsigned int sign : 1; //знак
    unsigned int SM : 2; //матрица
    unsigned int P : 1; //бит чётности
};
#pragma pack(pop)

#pragma pack(push, 1)
struct BCD //структура для нескольких параметров
{
    unsigned char address : 8; //адрес
    unsigned char : 3;
    unsigned char s : 6; //секунды
    unsigned char m : 6; //минуты
    unsigned char h : 5; //часы
    unsigned char : 1;
    unsigned char SM : 2; //матрица
    unsigned char P : 1; //бит чётности
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Discrete //структура для дискретных данных
{
    unsigned int address : 8; //адрес
    unsigned int SDI : 2; //инс
    unsigned int preparation : 1; //подготовка по ЗК
    unsigned int control : 1; //контроль
    unsigned int navigation : 1; //навигация
    unsigned int hypercomp : 1; //гиперкомпассирование
    unsigned int : 1;
    unsigned int re_start : 1; //повторный старт
    unsigned int scale : 3; //шкала подготовки
    unsigned int heating : 1; //исправность обогрева
    unsigned int temp : 1; //термостатирование
    unsigned int no_data : 1; //нет начальных данных
    unsigned int no_reception : 1; //нет приёма
    unsigned int ins : 1; //исправность ИНС
    unsigned int fast_readiness : 1; //готовность ускоренная
    unsigned int readiness : 1; //готовность
    unsigned int : 3;
    unsigned int SM : 2; //матрица
    unsigned int P : 1; //бит чётности
};
#pragma pack(pop)

#pragma pack(push, 1)
struct SRNS //структура для СРНС
{
    unsigned int address : 8; //адрес
    unsigned int start_data : 1; //запрос начальных данных
    unsigned int type_work_srns : 3; //тип рабочей СРНС
    unsigned int gps : 1; //альманах GPS
    unsigned int glonass : 1; //альманах ГЛОНАСС
    unsigned int operating_mode : 2; //режим работы
    unsigned int work_signal : 1; //подрежимы работы по сигналам СРНС
    unsigned int time : 1; //признак времени
    unsigned int : 2;
    unsigned int dif_work : 1; //дифференциальный режим измерений
    unsigned int : 1;
    unsigned int fail : 1; //отказ изделия
    unsigned int limit : 1; //порог сигнализации
    unsigned int system_cord : 2; //система координат
    unsigned int : 3;
    unsigned int matrix : 2; //матрица состояния
    unsigned int P : 1; //бит чётности
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DATA // структура слова "ДАТА"
{
    unsigned char address : 8; // адрес
    unsigned char : 2; 
    unsigned char year : 4; // год
    unsigned char month : 4; // месяц
    unsigned char day : 4; // день
    unsigned char matrix : 2; // матрица состояния
    unsigned char P : 1; // бит чётности
};
#pragma pack(pop)

union PackageArinc
{
    BNR mes1;
    BCD mes2;
    Discrete mes3;
    SRNS mes4;
    DATA mes5;
};

struct INS_prot
{
    PackageArinc width; // широта
    PackageArinc longitude; // долгота
    PackageArinc height; // высота
    PackageArinc course; // курс истинный
    PackageArinc pitch; // угол тангажа
    PackageArinc roll; // угол крена
    PackageArinc Vns; // скорость север/юг
    PackageArinc Vew; // скорость восток/запад
    PackageArinc Vvert; // скорость вертикальная инерциальная
    PackageArinc ax; // ускорение продольное
    PackageArinc az; // ускорение поперечное
    PackageArinc ay; // ускорение нормальное
    PackageArinc SS_INS; // слово состояния ИНС
};

struct SNS_prot
{
    PackageArinc H; // высота
    PackageArinc HDOP; // горизонтальный геометрический фактор
    PackageArinc VDOP; // вертикальный геометрический фактор
    PackageArinc PU; // путевой угол
    PackageArinc B; // текущая широта
    PackageArinc Bt; // текущая широта (точно)
    PackageArinc L; // текущая долгота
    PackageArinc Lt; // текущая долгота (точно)
    PackageArinc NP; // задержка выдачи обновленных НП относительно МВ
    PackageArinc UTC; // текущее время
    PackageArinc UTC_MB; // текущее время непрерывное между метками МВ
    PackageArinc Vh; // вертикальная скорость
    PackageArinc data; // дата
    PackageArinc P_SNS; // признаки СРНС
};

union protocol
{
    INS_prot prot1;
    SNS_prot prot2;
};


void Time()
{
    GetLocalTime(&st);
    printf("%d-%02d-%02d %02d:%02d:%02d.%03d ",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds);
}

int dec_to_oct(int n)
{
    int b, k = 1, c = 0;
    while (n > 0)
    {
        b = n % 8;
        n /= 8;
        c += b * pow(10, k - 1);
        b = 0;
        k++;
    }
    return(c);
}

double decode(double price, int high, int col, int val) {

    int shift = 28 - high;
    if (shift > 0)
    {
        val = val << shift;
    }

    double* bin = new double[col]();
    double s{};

    for (int i = 0; val > 0; i++)
    {
        bin[col - (i + 1)] = (val % 2);
        val /= 2;
    }

    for (int i = 0; i <= col - 1; i++) {
        s = s + bin[i] * (price / pow(2, i));
    }

    return s;
}

int CondINS{};
int CondSNS{};
int CondFi{};

void ConditionINS()
{
    while (CondINS == 0)
    {

    }
    if (CondINS == 1)
    {
        MessageBox(NULL, L"INS : preparation", L"Condition", MB_OK);
    }
    while (CondINS == 1)
    {

    }
    if (CondINS == 2)
    {
        MessageBox(NULL, L"INS : the value of phi is obtained", L"Condition", MB_OK);
    }
    while (CondINS == 2)
    {

    }
    if (CondINS == 3)
    {
        MessageBox(NULL, L"INS : navigation", L"Condition", MB_OK);
    }
}

void ConditionSNS()
{
    while (CondSNS == 0)
    {

    }
    if (CondSNS == 1)
    {
        MessageBox(NULL, L"SNS : synchronization", L"Condition", MB_OK);
    }
    while (CondSNS == 1)
    {

    }
    if (CondSNS == 2)
    {
        MessageBox(NULL, L"SNS : navigation", L"Condition", MB_OK);
    }
}

int __cdecl main(void)
{
    setlocale(LC_ALL, "Russian");

    WSADATA wsaData;
    SOCKET SendRecvSocket;  // Сокет для приема и передачи
    sockaddr_in ServerAddr{}, ClientAddr{};  // Адрес сервера и клиентов
    int err, ClientAddrSize = sizeof(ClientAddr);  // Размер ошибки и структуры адреса
    protocol* query{};  // Буфер приема

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Создание сокета для подключения к серверу
    SendRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // Настройка прослушивающего сокета
    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServerAddr.sin_port = htons(12346);
    err = bind(SendRecvSocket, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
    if (err == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(SendRecvSocket);
        WSACleanup();
        return 1;
    }

    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ConditionINS, NULL, NULL, NULL);
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ConditionSNS, NULL, NULL, NULL);

    while (true) {
        // Принятие клиентского сокета
        protocol data{};
        err = recvfrom(SendRecvSocket, (char*)&data, sizeof(protocol), 0, (sockaddr*)&ClientAddr, &ClientAddrSize);
        if (err > 0) {
            if (data.prot1.width.mes1.address == 1)
            {
                recvfrom(SendRecvSocket, (char*)&data, sizeof(protocol), 0, (sockaddr*)&ClientAddr, &ClientAddrSize);
                for (int i = 0; i < sizeof(adrr1); i++)
                {
                    if (dec_to_oct(data.prot1.SS_INS.mes3.address) == adrr1[i])
                    {
                        std::cout << "----------------------------------------" << std::endl;
                        Time();
                        std::cout << "ИНС Слово состояния\n";
                        std::cout << "address : " << dec_to_oct(data.prot1.SS_INS.mes3.address) << std::endl;
                        std::cout << "SDI : " << data.prot1.SS_INS.mes3.SDI << std::endl;
                        std::cout << "preparation : " << data.prot1.SS_INS.mes3.preparation << std::endl;
                        std::cout << "control : " << data.prot1.SS_INS.mes3.control << std::endl;
                        std::cout << "navigation : " << data.prot1.SS_INS.mes3.navigation << std::endl;
                        std::cout << "hypercomp : " << data.prot1.SS_INS.mes3.hypercomp << std::endl;
                        std::cout << "re_start : " << data.prot1.SS_INS.mes3.re_start << std::endl;
                        std::cout << "scale : " << data.prot1.SS_INS.mes3.scale << std::endl;
                        std::cout << "heating : " << data.prot1.SS_INS.mes3.heating << std::endl;
                        std::cout << "temp : " << data.prot1.SS_INS.mes3.temp << std::endl;
                        std::cout << "no_data : " << data.prot1.SS_INS.mes3.no_data << std::endl;
                        std::cout << "no_reception : " << data.prot1.SS_INS.mes3.no_reception << std::endl;
                        std::cout << "ins : " << data.prot1.SS_INS.mes3.ins << std::endl;
                        std::cout << "fast_readiness : " << data.prot1.SS_INS.mes3.fast_readiness << std::endl;
                        std::cout << "readiness : " << data.prot1.SS_INS.mes3.readiness << std::endl;
                        std::cout << "SM : " << data.prot1.SS_INS.mes3.SM << std::endl;
                        std::cout << "P : " << data.prot1.SS_INS.mes3.P << std::endl;
                        std::cout << "----------------------------------------" << std::endl;
                        std::cout << " " << std::endl;
                        if (data.prot1.SS_INS.mes3.preparation == 1)
                        {
                            if (data.prot1.SS_INS.mes3.no_data == 1)
                            {
                                CondINS = 1;
                            }
                            else
                            {
                                CondINS = 2;
                            }
                        }
                        else if (data.prot1.SS_INS.mes3.navigation == 1)
                        {
                            CondINS = 3;
                        }
                        continue;
                    }
                    if (dec_to_oct(data.prot1.width.mes1.address) == adrr1[i])
                    {
                        std::cout << "----------------------------------------" << std::endl;
                        Time();
                        std::cout << "ИНС Широта : " << decode(90, 28, 20, data.prot1.width.mes1.data) << std::endl;
                        std::cout << "----------------------------------------" << std::endl;
                        std::cout << " " << std::endl;
                        continue;
                    }
                    if (dec_to_oct(data.prot1.longitude.mes1.address) == adrr1[i])
                    {
                        std::cout << "----------------------------------------" << std::endl;
                        Time();
                        std::cout << "ИНС Долгота : " << decode(90, 28, 20, data.prot1.longitude.mes1.data) << std::endl;
                        std::cout << "----------------------------------------" << std::endl;
                        std::cout << " " << std::endl;
                        continue;
                    }
                    if (dec_to_oct(data.prot1.height.mes1.address) == adrr1[i])
                    {
                        std::cout << "----------------------------------------" << std::endl;
                        Time();
                        std::cout << "ИНС Высота : " << decode(19975.3728, 28, 19, data.prot1.height.mes1.data) << std::endl;
                        std::cout << "----------------------------------------" << std::endl;
                        std::cout << " " << std::endl;
                        continue;
                    }
                }
            }
            else if (data.prot1.width.mes1.address == 2)
            {
                recvfrom(SendRecvSocket, (char*)&data, sizeof(protocol), 0, (sockaddr*)&ClientAddr, &ClientAddrSize);
                for (int i = 0; i < sizeof(adrr2); i++)
                {
                    if (dec_to_oct(data.prot2.P_SNS.mes4.address) == adrr2[i])
                    {
                        std::cout << "----------------------------------------" << std::endl;
                        Time();
                        std::cout << "СНС Слово состояние\n";
                        std::cout << "address : " << dec_to_oct(data.prot2.P_SNS.mes4.address) << std::endl;
                        std::cout << "star_data : " << data.prot2.P_SNS.mes4.start_data << std::endl;
                        std::cout << "type_work_srns : " << data.prot2.P_SNS.mes4.type_work_srns << std::endl;
                        std::cout << "gps : " << data.prot2.P_SNS.mes4.gps << std::endl;
                        std::cout << "glonass : " << data.prot2.P_SNS.mes4.glonass << std::endl;
                        std::cout << "operating_mode : " << data.prot2.P_SNS.mes4.operating_mode << std::endl;
                        std::cout << "work_signal : " << data.prot2.P_SNS.mes4.work_signal << std::endl;
                        std::cout << "time : " << data.prot2.P_SNS.mes4.time << std::endl;
                        std::cout << "dif_work : " << data.prot2.P_SNS.mes4.dif_work << std::endl;
                        std::cout << "fail : " << data.prot2.P_SNS.mes4.fail << std::endl;
                        std::cout << "limit : " << data.prot2.P_SNS.mes4.limit << std::endl;
                        std::cout << "system_cord : " << data.prot2.P_SNS.mes4.system_cord << std::endl;
                        std::cout << "matrix : " << data.prot2.P_SNS.mes4.matrix << std::endl;
                        std::cout << "P : " << data.prot2.P_SNS.mes4.P << std::endl;
                        std::cout << "----------------------------------------" << std::endl;
                        std::cout << " " << std::endl;
                        if (data.prot2.P_SNS.mes4.work_signal == 1)
                        {
                            CondSNS = 1;
                        }
                        else if (data.prot2.P_SNS.mes4.work_signal == 0)
                        {
                            CondSNS = 2;
                        }
                        continue;
                    }
                    if (dec_to_oct(data.prot2.PU.mes1.address) == adrr2[i])
                    {
                        std::cout << "----------------------------------------" << std::endl;
                        Time();
                        std::cout << "СНС Путевой угол : " << decode(90, 28, 15, data.prot2.PU.mes1.data) << std::endl;
                        std::cout << "----------------------------------------" << std::endl;
                        std::cout << " " << std::endl;
                        continue;
                    }
                    if (dec_to_oct(data.prot2.Vh.mes1.address) == adrr2[i])
                    {
                        std::cout << "----------------------------------------" << std::endl;
                        Time();
                        std::cout << "СНС Вертикальная скорость : " << decode(16384, 28, 15, data.prot2.Vh.mes1.data) << std::endl;
                        std::cout << "----------------------------------------" << std::endl;
                        std::cout << " " << std::endl;
                        continue;
                    }
                }
            }
            else if (data.prot1.width.mes1.address == 3)
            {
            Time();
            std::cout << "ИНС: запущена отправка" << std::endl;
            }
            else if (data.prot1.width.mes1.address == 4)
            {
            Time();
            std::cout << "СНС: запущена отправка" << std::endl;
            }
        }
        else {
            printf("recv failed: %d\n", WSAGetLastError());
            closesocket(SendRecvSocket);
            WSACleanup();
            return 1;
        }
    }
}