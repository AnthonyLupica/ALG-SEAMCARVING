/* 
    seamCarving.cpp
*/

#include <iostream> 
#include <fstream>
#include <vector>
#include <string> 
#include <sstream>

using std::cout;
using std::cerr;
using std::endl;
using std::getline;
using std::ifstream;
using std::vector;
using std::string;
using std::stringstream;

vector<vector<int>> initImageMap(const string &filename);
void displayMatrix(const vector<vector<int>> matrix);

int main(int argc, char* argv[]) 
{
    cout << " ______________________________________________________\n";
    cout << "|                                                      |\n";
    cout << "| 3460:435/535 Algorithms Project Three - Seam Carving |\n";
    cout << "|______________________________________________________|\n\n";

    // VALIDATE ARGUMENTS
    if(argc != 4) 
    {
        cerr << "error: invalid command-line arguments\n"
             << "format of valid program invocation: ./a [pgm image file] [# vertical seams to remove] [# horizontal seams to remove]\n";
    }

    // INITIALIZE THE IMAGE MAP 
    vector<vector<int>> I = initImageMap(argv[1]);

    displayMatrix(I);

    return 0;
}

// <Summary> A vector<vector<int>> is populated with the image values contained in the pgm file, 'filename' </Summary> 
// <Param name='filename'> name of a file with .pgm extension </Param> 
// <Return> The vector<vector<int>> by value </Return> 
// <Assumptions> 
//      initImageMap assumes the pgm file format outlined in the project description is rigorously adhered to. 
//      noteably, a hard assumption is made that one optional comment is in the file, necessarily on line two (if it exists)
// </Assumptions>
vector<vector<int>> initImageMap(const string &filename)
{
    ifstream pgmInputFile(filename);
 
    // validate good connection to the input file
    if (!pgmInputFile) 
    {
        cerr << "error: could not open file '" << filename << "'\n"
             << "check the file name is correct and the file is located at the same directory level as the executable\n";
        exit(1);
    }

    // #region parse_to_data 
    string temp_line;
    
    // handle file format line
    getline(pgmInputFile, temp_line); // "P2"
    if (temp_line != "P2") 
    { 
        // this program handles only P2 images, meaning colors must be in greyscale
        cerr << "error: invalid pgm file format\n"
             << "file format was read as '" << temp_line << "', while the only supported format is 'P2'\n";
        exit(1);
    }

    // @Note - crucially, the code here assumes line two of the header is the singular comment-line, or there are no comments in the file at all
    if (pgmInputFile.peek() == '#') 
    { 
        getline(pgmInputFile, temp_line); // skip optional comment
    }

    getline(pgmInputFile, temp_line); // columns X rows

    // parse out the column and row count from this line
    int columns = 0, rows = 0;
    stringstream dimensions;      
    dimensions << temp_line;       // read entire line into string stream
    dimensions >> columns >> rows; // write whitespace seperated values into variables

    // something went wrong if these are still 0
    if (columns == 0 || rows == 0)
    {
        // an error occured in reading the image dimensions
        cerr << "error: a problem occured in reading the pgm file dimensions\n"
             << "please ensure the data format outlined in the project description is strictly adhered to \n";
        exit(1);
    }

    getline(pgmInputFile, temp_line); // maximum greyscale value
    // #endregion

    // #region parse_data
    // read raw pixel data into a string, and subsequently into a stringstream
    string pixelData;
    stringstream ssPixelData;
    while (!pgmInputFile.eof())
    {
        // grab a row of data
        getline(pgmInputFile, temp_line); 

        // append to the result string
        pixelData += temp_line;
    }
    ssPixelData << pixelData;

    // populate a 2D vector with the data
    vector<vector<int>> result;
    int pixel = 0;
    for (int i = 0; i < rows; ++i)
    {
        vector<int> temp;
        for (int j = 0; j < columns; ++j)
        {
            ssPixelData >> pixel;
            
            temp.push_back(pixel);
        }
        
        result.push_back(temp);
    }
    // #endregion

    return result;
}

void displayMatrix(const vector<vector<int>> matrix)
{
    for (vector<int> row : matrix)
    {
        for (int pixel : row)
        {
            if (pixel < 100 && pixel >= 10)
            {
                cout << "  " << pixel << " ";
            }
            else if (pixel < 10)
            {
                cout <<  "   " << pixel << " ";
            }
            else 
            {
                cout <<  " " << pixel << " ";
            }
        }

        cout << "\n";
    }

    return;
}
