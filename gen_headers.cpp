#include <fstream>
#include <sstream>

using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::endl;

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    ifstream language("../language.txt");
    if (!language.good()) return 1;
    stringstream ss;
    ss << language.rdbuf();
    auto lang = ss.str();

    ofstream out(argv[1]);
    out << "#ifndef LISPY_GENEATED_HPP\n#define LISPY_GENEATED_HPP\n\n";

    out << "char language_grammar[] = {";

    for (size_t i = 0; i < lang.size(); i++) {
        out << static_cast<uint>(lang[i]);

        if (i != lang.size() - 1) {
            out << ", ";
        }
    }

    out << "};\n\n";
    out << "#endif // LISPY_GENEATED_HPP" << endl;
}
