#include <fstream>
#include <sstream>

using std::ifstream;
using std::ofstream;
using std::stringstream;
using std::endl;

int main(int argc, char* argv[]) {
    if (argc < 2) return 1;

    ifstream language("language.txt");
    stringstream ss;
    ss << language.rdbuf();

    ofstream out(argv[1]);
    out << "#ifndef LISPY_GENEATED_HPP\n#define LISPY_GENEATED_HPP" << endl;
    out << "#endif // LISPY_GENEATED_HPP";
}
