#include <fstream>
#include <sstream>
#include <string>

using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::endl;

bool write_file(ofstream &out, const string &var_name, const string &filename) {
    ifstream file(filename);
    if (!file.good()) return false;
    stringstream ss;
    ss << file.rdbuf();
    auto contents = ss.str();

    out << "char " << var_name << "[] = {";

    for (size_t i = 0; i < contents.size(); i++) {
        out << static_cast<uint>(contents[i]);

        if (i != contents.size() - 1) {
            out << ", ";
        }
    }

    out << ", 0};\n\n";

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    ofstream out(argv[1]);
    out << "#ifndef LISPY_GENERATED_HPP\n#define LISPY_GENERATED_HPP\n\n";

    if (write_file(out, "language_grammar", "../language.txt")) {
        if (!write_file(out, "prelude", "../prelude.lspy")) {
            return 1;
        }
    } else {
        return 1;
    }

    out << "#endif // LISPY_GENERATED_HPP" << endl;
    return 0;
}
