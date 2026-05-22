#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
using namespace std;

#define OUTPUT

mt19937 rng(random_device{}());

#pragma pack(push, 1)
struct file_header {
    unsigned short id;
    unsigned int f_size;
    unsigned short rez_1, rez_2;
    unsigned int bm_offset;
};

struct v3_header {
    unsigned int h_size;
    unsigned int width;
    unsigned int height;
    unsigned short planes, bit_per_pixel;
    unsigned int compression;
    unsigned int size_image;
    unsigned int h_res;
    unsigned int v_res;
    unsigned int clr_used;
    unsigned int clr_imp;
};

struct color_info {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
    unsigned char temp;
};
#pragma pack(pop)

vector<double> GenerateP(const int& m) {
    uniform_real_distribution<double> dist(0.0, 1.0);
    double u = 0, pi = 0, eps = 1e-8;
    vector<double> p(m);
    for (int i = 0; i < m; ++i) {
        u = dist(rng);
        pi = pow(sin(M_PI * u / 2.0), 2);
        if (pi < eps) pi = eps;
        if (pi > (1.0 - eps)) pi = 1.0 - eps;
        p[i] = pi;
    }
    return p;
}

vector<vector<int>> GenerateX(const int& n, const int& m,
                              const vector<double>& p) {
    vector<vector<int>> x(n, vector<int>(m));
    uniform_real_distribution<double> dist(0.0, 1.0);
    double r = 0;
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            r = dist(rng);
            x[j][i] = (r < p[i]) ? 1 : 0;
        }
    }
    return x;
}

void InsertFinger(const string& file, const vector<int>& x,
                  const string& file_o) {
    file_header header;
    v3_header v3header;
    color_info color_table[256];

    std::ifstream file_in(file, std::ios::binary);
    if (!file_in) {
        cout << "error file orig";
        throw std::exception();
    }
    file_in.read(reinterpret_cast<char*>(&header), sizeof(file_header));
    file_in.read(reinterpret_cast<char*>(&v3header), sizeof(v3_header));
    file_in.read(reinterpret_cast<char*>(color_table),
                 256 * sizeof(color_info));
    file_in.seekg(header.bm_offset, std::ios::beg);

    std::ofstream file_out(file_o, std::ios::binary);
    if (!file_out) {
        cout << "error out file";
        throw std::exception();
    }
    file_out.write(reinterpret_cast<char*>(&header), sizeof(file_header));
    file_out.write(reinterpret_cast<char*>(&v3header), sizeof(v3_header));
    file_out.write(reinterpret_cast<char*>(color_table),
                   256 * sizeof(color_info));

    vector<int> indx(v3header.width);
    iota(indx.begin(), indx.end(), 0);
    size_t seed = hash<string>{}("KEY");
    mt19937 rng2((uint32_t)seed);
    shuffle(indx.begin(), indx.end(), rng2);

    int bpr = floor((v3header.bit_per_pixel * v3header.width + 31) / 32) * 4,
        point = 0, siz = x.size(), rs = 0;
    char buf[bpr];
    do {
        file_in.read(reinterpret_cast<char*>(buf), bpr);
        for (int i = 0; i < v3header.width; i++) {
            if (point == siz) break;
            buf[indx[i]] = (buf[indx[i]] & 254) | (x[point] & 1);
            point += 1;
        }
        rs = file_in.gcount();
        file_out.write(reinterpret_cast<char*>(buf), bpr);
    } while (rs > 0);

    file_in.close();
    file_out.close();
}

vector<int> TakeFinger(const string& file, const int& m) {
    vector<int> x;

    file_header header;
    v3_header v3header;
    color_info color_table[256];

    std::ifstream file_in(file, std::ios::binary);
    if (!file_in) {
        cout << "error file orig";
        throw std::exception();
    }
    file_in.read(reinterpret_cast<char*>(&header), sizeof(file_header));
    file_in.read(reinterpret_cast<char*>(&v3header), sizeof(v3_header));
    file_in.read(reinterpret_cast<char*>(color_table),
                 256 * sizeof(color_info));
    file_in.seekg(header.bm_offset, std::ios::beg);

    vector<int> indx(v3header.width);
    iota(indx.begin(), indx.end(), 0);
    size_t seed = hash<string>{}("KEY");
    mt19937 rng2((uint32_t)seed);
    shuffle(indx.begin(), indx.end(), rng2);

    int bpr = floor((v3header.bit_per_pixel * v3header.width + 31) / 32) * 4,
        point = 0, rs = 0;
    char buf[bpr];

    do {
        file_in.read(reinterpret_cast<char*>(buf), bpr);
        for (int i = 0; i < v3header.width; i++) {
            if (point == m) break;
            x.push_back(buf[indx[i]] & 1);
            point += 1;
        }
        rs = file_in.gcount();
    } while (rs > 0 && point < m);

    file_in.close();
    return x;
}

double G(const int& y, const int& x, const double& p) {
    if (y == 1 && x == 1) return sqrt((1 - p) / p);
    if (y == 0 && x == 0) return sqrt(p / (1 - p));
    if (y == 0 && x == 1) return -sqrt(p / (1 - p));
    return -sqrt((1 - p) / p);
}

int main() {
    double eps = 0.1;
    int n = 10, c = 0, c_real;
    vector<int> id(n);
    iota(id.begin(), id.end(), 0);
#if defined(OUTPUT)
    cout << "Input c -> ";
#endif
    cin >> c;
#if defined(OUTPUT)
    cout << "Input c_real -> ";
#endif
    cin >> c_real;
    int m = static_cast<int>(ceil(M_PI * M_PI * c * c * log(1.0 / eps) / 2.0));
    if (m < 100) m = 100;
    double z = sqrt(2 * m * log(1 / eps));

#if defined(OUTPUT)
    cout << "n = 10, e = 0.1, m = " << m << ", z = " << z
         << "\nCreate marix...";
#endif
    vector<double> p = GenerateP(m);
    vector<vector<int>> x = GenerateX(n, m, p);
#if defined(OUTPUT)
    cout << "complete\n";

    cout << "Create author images...\n";
#endif
    string str;
    for (int i = 0; i < n; ++i) {
#if defined(OUTPUT)
        cout << "Input author_" << i << " image -> ";
#endif
        cin >> str;
        // str="../../bmp/12.bmp";
        InsertFinger(str, x[i], ("author_" + to_string(i) + ".bmp"));
    }
#if defined(OUTPUT)
    cout << "...complete\n";
#endif

    shuffle(id.begin(), id.end(), rng);
#if defined(OUTPUT)
    cout << "Id for atack - ";
#endif
    vector<vector<int>> temp_x;
    vector<int> pirates;
    for (int i = 0; i < c_real; ++i) {
#if defined(OUTPUT)
        cout << id[i] << ' ';
#endif
        pirates.push_back(id[i]);
        str = "author_" + to_string(id[i]) + ".bmp";
        temp_x.push_back(TakeFinger(str, m));
    }

#if defined(OUTPUT)
    cout << "\nCoalition atack...";
#endif
    vector<int> y(m);
    int val[2];
    for (int j = 0; j < m; ++j) {
        val[0] = 0;
        val[1] = 0;
        for (int i = 0; i < c_real; ++i) val[temp_x[i][j]] += 1;
        if (val[0] > 0 && val[1] > 0)
            y[j] = rand() % 2;
        else if (val[0] == 0 && val[1] > 0)
            y[j] = 1;
        else
            y[j] = 0;
    }
    InsertFinger(("author_" + to_string(id[0]) + ".bmp"), y, "pirate.bmp");
#if defined(OUTPUT)
    cout << "complete\n";
#endif

    vector<double> s(n);
    y = TakeFinger("pirate.bmp", m);
#if defined(OUTPUT)
    cout << "Start analyze:\n";
#endif
    int count_pirate = pirates.size(), count_find = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) s[i] += G(y[j], x[i][j], p[j]);
        cout << "author " << i << " = " << s[i];
        if (s[i] >= z) {
            cout << " pirate";
            if (find(pirates.begin(), pirates.end(), i) != pirates.end())
                count_find += 1;
        }
        cout << '\n';
    }
    cout << "Find " << count_find << '/' << count_pirate << " pirates, its "
         << (count_find / (double)count_pirate) * 100.0 << "%\n";
    return 0;
}