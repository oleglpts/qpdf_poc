#include <ctime>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <qpdf/QPDF.hh>
#include <qpdf/Buffer.hh>
#include <qpdf/Constants.h>
#include <qpdf/PointerHolder.hh>

using namespace std;

/**
 * Skip some filters
 *
 * @param name - filter name
 * @return stream decode level
 */
qpdf_stream_decode_level_e getLevel(const string& name) {
    if ((name == "/DCTDecode") || name == ("/JPXDecode"))  // CCITTFaxDecode not supported))
        return qpdf_dl_none;
    else
        return  qpdf_dl_all;
}

/**
 * Print dictionary
 *
 * @param dictionary - dictionary map
 * @param old_level - old stream decode level
 * @return stream decode level
 */
qpdf_stream_decode_level_e printDictionary(map<std::string, QPDFObjectHandle> dictionary,
        qpdf_stream_decode_level_e old_level) {
    cout << "<< ";
    qpdf_stream_decode_level_e level = old_level;
    for (auto &key : dictionary) {
        cout << key.first;
        if (key.second.isName()) {
            auto name = key.second.getName();
            if (level == qpdf_dl_all)
                level = getLevel(name);
            cout << " " << name << " ";
        }
        else if (key.second.isNumber()) {
            cout << " " << key.second.getNumericValue() << " ";
        }
        else if (key.second.isArray()) {
            for (int i = 0; i < key.second.getArrayAsVector().size(); ++i) {
                cout << " ";
                auto item = key.second.getArrayItem(i);
                if (item.isDictionary()) {
                    auto new_level = printDictionary(key.second.getArrayItem(i).getDictAsMap(), level);
                    if (level == qpdf_dl_all)
                        level = new_level;
                }
                else if (item.isName()) {
                    auto name = item.getName();
                    if (level == qpdf_dl_all)
                        level = getLevel(name);
                    cout << name;
                }
                cout << " ";
            }
        }
    }
    cout << ">>";
    return level;
}

/**
 * QPDF library example
 *
 * Used QPDF library (http://qpdf.sourceforge.net/) licensed under the GNU Lesser General Public License
 * Documentation: http://qpdf.sourceforge.net/files/qpdf-manual.pdf
 * qpdf utility command line: qpdf --show-object=n --{filtered | raw}-stream-data pdf_file_name
 */
int main(int argc, const char* argv[]) {
    // Start
    clock_t begin = clock();
    // Check parameters
    if(argc - 2) {
        cout << "input file path required" << endl;
        return 1;
    }
    cout << "Parsing file '" << argv[1] << "':" << endl;
    try {
        // Parse
        QPDF doc;
        doc.processFile(argv[1]);
        // Objects cycle
        string dir = string(argv[1]).append(".qpdf_out");
        rmdir(dir.c_str());
        mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        for (auto object : doc.getAllObjects()) {
            if (object.isStream()) {
                cout << "    Object " << object.getObjectID() << " has stream ";
                // Print dictionary
                auto level = qpdf_dl_all;
                level = printDictionary(object.getDict().getDictAsMap(), level);
                // Output file name
                ostringstream file_name;
                file_name << dir << "/pdf_";
                file_name.width(4);
                file_name.fill ('0');
                file_name << object.getObjectID() << "_0.dat";
                // Stream output
                PointerHolder<Buffer> stream;
                try {
                    stream = level == qpdf_dl_none ? object.getRawStreamData() : object.getStreamData(level);
                    if (level == qpdf_dl_none)
                        cout << " (filter omitted)";
                }
                catch (exception &e) {
                    stream = object.getRawStreamData();
                    cout << " (filter not supported)";
                }
                ofstream stream_file(file_name.str());
                auto stream_data = stream.getPointer()->getBuffer();
                for (unsigned long i=0; i < stream.getPointer()->getSize(); i++)
                    stream_file << stream_data[i];
                cout << endl;
            }
        }
    }
    // Error handle
    catch (exception &e) {
        cout << endl << "Error: " << e.what() << endl;
        return 1;
    }
    // Execution time output
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    cout << endl << "Execution time: " << time_spent << " sec." << endl;
    return 0;
}
