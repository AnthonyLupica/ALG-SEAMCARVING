/* 
    seamCarving.cpp

    run with -> ./a [pgm image file] [# vertical seams to remove] [# horizontal seams to remove]

    seam carving changes the size of an image by removing the least visible pixels in the image. 
    the visibility of a pixel can be defined using an energy function. Seam carving can be done by finding a 
    one-pixel wide path of lowest energy crossing the image from top to bottom (vertical path) or 
    from left to right (horizontal path) and removing the path (seam).

    Assumptions: 
        The pgm file provided adheres to the following format...
        
        P2                       ; P2 designating greyscale image 
        # Created by IrfanView   ; optional comment
        y x                      ; columns(y) by rows(x)
        255                      ; upper bound on values
        *                        ; pixel data begins here 
        *
        * 

    Note: Several print statements have been commented out in main. These visualize each step of the process;
          uncomment them out for debugging help.
*/

#include <iostream> 
#include <fstream>
#include <vector>
#include <string> 
#include <sstream>
#include <cmath> 
#include <algorithm>
#include <utility> 

using std::cout;
using std::cerr;
using std::endl;
using std::getline;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::string;
using std::stringstream;

// CORE 

vector<vector<int>> initImageMap(const string &filename);
vector<vector<int>> initEnergyMap(const vector<vector<int>> &imageMap);
vector<vector<int>> initCumulativeEnergyMap(const vector<vector<int>> &energyMap);
void seamCarver(vector<vector<int>> &imageMap, const vector<vector<int>> &cumulativeEnergyMap);

// HELPERS

void transposeMap(vector<vector<int>> &imageMap);
void displayMap(const vector<vector<int>> &map);
void displayTranspose(const vector<vector<int>> &map);
void validateCarveRequests(const vector<vector<int>> &imageMap, int num_vertical_seams, int num_horizontal_seams);
void writeResults(const vector<vector<int>> &imageMap, const string &filename);

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
        exit(1);
    }

    // INITIALIZE THE IMAGE MAP 
    string fullname = string(argv[1]);
    vector<vector<int>> I = initImageMap(fullname);

    // validate command-line args for vertical/horizontal carve requests
    int num_vertical_seams = atoi(argv[2]);
    int num_horizontal_seams = atoi(argv[3]);
    validateCarveRequests(I, num_vertical_seams, num_horizontal_seams);

    // cout << "'" << argv[1] << "' --> Initial Image Map:\n";
    // displayMap(I);

    // CARVE THE REQUESTED NUMBER OF VERTICAL SEAMS
    for (int i = 1; i <= num_vertical_seams; ++i)
    {
        cout << "\n[C][A][R][V][I][N][G] [V[E][R][T][I][C][A][L] [S][E][A][M] [" << i << "]\n";

        // cout << "\nInitial Image Map:\n";
        // displayMap(I);

        // INITIALIZE THE ENERGY MAP
        vector<vector<int>> E = initEnergyMap(I);
        
        // cout << "\nEnergy Map: \n";
        // displayMap(E);

        // INITIALIZE THE CUMULATIVE ENERGY MAP 
        vector<vector<int>> CE = initCumulativeEnergyMap(E);

        // cout << "\nCumulative Energy Map: \n";
        // displayMap(CE);

        // CARVE OUT A SEAM
        seamCarver(I, CE); 

        // cout << "\nSeam-Carved Image Map: \n";
        // displayMap(I);
    }

    // CARVE THE REQUESTED NUMBER OF HORIZONATL SEAMS
    if (num_horizontal_seams > 0)
    {    
        // if-block protects against unecessarily transposing the image map

        transposeMap(I); // transpose the map to reuse the vertical seam carver for horizontal seams
        for (int i = 1; i <= num_horizontal_seams; ++i)
        {
            cout << "\n[C][A][R][V][I][N][G] [H[O][R][I][Z][O][N][T][A][L] [S][E][A][M] [" << i << "]\n";

            // cout << "\nInitial Image Map:\n";
            // displayTranspose(I);

            // INITIALIZE THE ENERGY MAP
            vector<vector<int>> E = initEnergyMap(I);
            
            // cout << "\nEnergy Map: \n";
            // displayTranspose(E);

            // INITIALIZE THE CUMULATIVE ENERGY MAP 
            vector<vector<int>> CE = initCumulativeEnergyMap(E);

            // cout << "\nCumulative Energy Map: \n";
            // displayTranspose(CE);

            // CARVE OUT A SEAM
            seamCarver(I, CE); 

            // cout << "\nSeam-Carved Image Map: \n";
            // displayTranspose(I);
        }
        transposeMap(I); // undo the transpose
    }

    // WRITE RESULTS TO FILE

    // get the raw file name
    string rawname = fullname.substr(0, fullname.find_last_of("."));

    // derive file name to write to (./a example.pgm 10 5  --->  example_processed_10_5.pgm)
    string fileToWrite = rawname + "_processed_" + std::to_string(num_vertical_seams) + "_" + std::to_string(num_horizontal_seams) + ".pgm";

    // write the processed image to fileToWrite
    writeResults(I, fileToWrite);

    cout << "\nEND PROCESSING\n";
    cout << "Results written to '" << fileToWrite << "' \n";
    
    return 0;
}

/// @brief A 2D vector of integers is populated with the image pixel values comprising the pgm image file, 'filename'.
/// @param filename Name of a file with a pgm extension.
/// @return The resultant image map by value.
/// @note initImageMap assumes the pgm file format outlined in the project description is rigorously adhered to. 
///       Noteably, a hard assumption is made that one optional comment is in the file, necessarily on line two (if it exists).
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

    // #REGION parse_to_data 
    string temp_line;
    
    // handle file format line
    getline(pgmInputFile, temp_line); // "P2"
    if (temp_line != "P2") 
    { 
        // this program handles only P2 images, meaning colors must be in greyscale
        cerr << "error: invalid pgm file format\n"
             << "file format was read as '" << temp_line << "', while the supported format is 'P2' for a PGM file\n";
        exit(1);
    }

    // @NOTE - crucially, the code here assumes line two of the header is the singular comment-line, or there are no comments in the file at all
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
    int maxPixelValue = stoi(temp_line);
    // #ENDREGION

    // #REGION parse_data
    // read raw pixel data into a string, and subsequently into a stringstream
    string pixelData;
    stringstream ssPixelData;
    while (!pgmInputFile.eof())
    {
        // grab a row of data
        getline(pgmInputFile, temp_line); 

        // if temp_line does not end in a whitespace we will append one ourself 
        if (temp_line[temp_line.size() - 1] != ' ')
        {
            temp_line.append(" ");
        }

        // append to the result string
        pixelData += temp_line;
    }
    ssPixelData << pixelData;

    // populate a 2D vector with the data
    vector<vector<int>> result;
    int pixel = 0;
    for (int i = 0; i < rows; ++i)
    {
        // outer-for iterates over rows

        vector<int> rowResult;
        for (int j = 0; j < columns; ++j)
        {
            // inner-for iterates over individual pixels in each row
            ssPixelData >> pixel;

            // ensure the pixel is within the valid range of values
            if (pixel > maxPixelValue || pixel < 0)
            {
                cerr << "error: a pixel value exists in the image data which falls outside the given acceptable range of [0, " << maxPixelValue << "]\n";
                exit(1);
            }
            
            rowResult.push_back(pixel);
        }
        
        result.push_back(rowResult);
    }
    // #ENDREGION

    return result;
}

/// @brief A 2D vector of integers is populated with pixel energy values using an image map produced by initImageMap.
/// @param imageMap A 2D vector containing the pixel data of a pgm file
/// @return The resultant energy map by value.
vector<vector<int>> initEnergyMap(const vector<vector<int>> &imageMap)
{
    vector<vector<int>> result;
    for (int i = 0; i < imageMap.size(); ++i)
    {
        // outer-for iterates over rows

        vector<int> rowResult;
        for (int j = 0; j < imageMap[i].size(); ++j) 
        {
            // inner-for iterates over individual pixels in each row
            
            // #REGION find ΔI along X axis 
            // bounds checking X
            int left = (j - 1) >= 0 ? imageMap[i][j - 1] : imageMap[i][j]; 
            int right = (j + 1) < imageMap[i].size() ? imageMap[i][j + 1] : imageMap[i][j];

            int changeX = abs(imageMap[i][j] - left) + abs(imageMap[i][j] - right);
            // #ENDREGION

            // #REGION find ΔI along Y axis 
            // bounds checking Y
            int up = (i - 1) >= 0 ? imageMap[i - 1][j] : imageMap[i][j];
            int down = (i + 1) < imageMap.size() ? imageMap[i + 1][j] : imageMap[i][j];

            int changeY = abs(imageMap[i][j] - up) + abs(imageMap[i][j] - down);
            // #ENDREGION
            
            // store the energy of the pixel
            rowResult.push_back(changeX + changeY);
        }

        result.push_back(rowResult);
    }
    
    return result;
}

/// @brief A 2D vector of integers is populated with cumulative energy values using an energy matrix.
/// @param energyMap The energy map which is used to derive the CE map.
/// @return The resultant cumulative energy map by value.
/// @note The lowest energy value in the final row of a CE matrix represents the pixel 
///       which ends the lowest energy seam.
vector<vector<int>> initCumulativeEnergyMap(const vector<vector<int>> &energyMap)
{
    // initialize result with the contents of the energyMap
    vector<vector<int>> result (energyMap);

    /*  objective: for each pixel, act as though this is the end of the seam.
                   write to result at this indexed pixel, the "cumulative energy"
                   of itself and the pixel it would have come from. 
        EX: X comes from 1 (the lowest energy ancestor)
        +---+---+---+---+
        | 1 | 2 | 3 | 4 |
        +---+---+---+---+
        |   | X |   |   |
        +---+---+---+---+

        from the perspective of X, we are looking at ... 
            energyMap[i - 1][j - 1], energyMap[i - 1][j], and energyMap[i - 1][j + 1]
        
        bounds checking will need to be done on j
    */

    for (int i = 1; i < result.size(); ++i)
    {
        // outer-for iterates over rows (begin iterating on second row)

        for (int j = 0; j < result[i].size(); ++j) 
        {
            // inner-for iterates over individual pixels in each row
            
            //#REGION bounds checking
            // create a list of up to 3 (or 2) seam origin pixels in the row above that can be connected
            // to the seam pixel in the current row.
            vector<int> nextSeamPixelCandidates;

            // validate bounds of upper-left
            if ((j - 1) >= 0) 
            { 
                nextSeamPixelCandidates.push_back(result[i - 1][j - 1]);
            }

            // no need to validate upper
            nextSeamPixelCandidates.push_back(result[i - 1][j]);

            // validate bounds of upper-right
            if ((j + 1) < energyMap[i].size()) 
            { 
                nextSeamPixelCandidates.push_back(result[i - 1][j + 1]);
            }
            //#ENDREGION

            // at this pixel, add to its energy the minimum energy contained within nextSeamPixelCandidates
            result[i][j] += *std::min_element(nextSeamPixelCandidates.begin(), nextSeamPixelCandidates.end());
        }
    }

    return result;
}

/// @brief Given the image map and its cumulative energy map, "carve out" the lowest energy seam 
///        from the image map.
/// @param imageMap The image map to be modified by the seamCarver.
/// @param cumulativeEnergyMap The CE map to be traced-back to determine the lowest energy seam.
void seamCarver(vector<vector<int>> &imageMap, const vector<vector<int>> &cumulativeEnergyMap)
{
    // modify imageMap by identifying the pixel in each row that is an element of the 
    // lowest energy seam, shifting it the end of the row, and popping back that element to carve out the seam

    // each element of seam_column_indices corresponds to a row in the image map, 
    // and contains the column index of the seam pixel in that row.
    int seam_column_indices[cumulativeEnergyMap.size()] = { -1 }; 

    // get the index of the seam-ending pixel and put at the end of the seam pixel list
    // the seam-ending pixel is the element in the final row of the cumulativeEnergyMap with the lowest energy
    int num_rows = cumulativeEnergyMap.size();                                                                                
    auto seam_end_itr = std::min_element(cumulativeEnergyMap[num_rows - 1].begin(), cumulativeEnergyMap[num_rows - 1].end()); 
    int seam_end_index = std::distance(cumulativeEnergyMap[num_rows - 1].begin(), seam_end_itr);                              
    seam_column_indices[num_rows - 1] = seam_end_index;

    // iterate in reverse-row order, descending the seam
    // for each iteration, first remove the seam pixel for that row 
    // and next trace-back the seam to find the index of the seam pixel connected to it for the next iteration
    for (int i = cumulativeEnergyMap.size() - 1; i >= 0; --i)
    {
        //#REGION remove the seam_pixel for this row
        // Remove the seam pixel from the current row by swapping it with adjacent pixels
        // until it reaches the end of the row, and then "snipping" it off from the end.
        int seam_pixel_index = seam_column_indices[i];
        while (seam_pixel_index + 1 < cumulativeEnergyMap[i].size())
        {
            // while-loop swaps the seam pixel until it is at the end of the row 
            std::swap(imageMap[i][seam_pixel_index], imageMap[i][seam_pixel_index + 1]);
            ++seam_pixel_index;
        }
        imageMap[i].pop_back();
        //#ENDREGION
        
        //#REGION find out what the next seam pixel index is for the next iteration
        if ((i - 1) >= 0)
        {
            // if-block only reached if trace-back step doesn't take us out of bounds

            //#REGION bounds checking
            // create a candidate list of seam pixel indices
            // that can connect to the seam pixel in the current row.

            // declarations
            vector<int> nextSeamPixelCandidates; 
            int indexUpperLeft = seam_column_indices[i] - 1;
            int indexUpper = seam_column_indices[i];
            int indexUpperRight = seam_column_indices[i] + 1;

            // validate bounds of upper-left
            bool upperLeftIsIncluded = false;
            if ((indexUpperLeft) >= 0) 
            { 
                // the connecting seam pixel might be the upper-left pixel
                nextSeamPixelCandidates.push_back(cumulativeEnergyMap[i - 1][indexUpperLeft]);
                upperLeftIsIncluded = true;
            }

            // no need to validate upper as it will never be out of bounds
            nextSeamPixelCandidates.push_back(cumulativeEnergyMap[i - 1][indexUpper]);

            // validate bounds of upper-right
            bool upperRightIsIncluded = false;
            if ((indexUpperRight) < cumulativeEnergyMap[i].size()) 
            { 
                // the connecting seam pixel might be the upper-right pixel
                nextSeamPixelCandidates.push_back(cumulativeEnergyMap[i - 1][indexUpperRight]);
                upperRightIsIncluded = true;
            }

            // among the candidate seam pixels, find the one with the lowest energy
            int seam_traceback_value = *std::min_element(nextSeamPixelCandidates.begin(), nextSeamPixelCandidates.end()); 
            
            // find the index in the trace-back row of this "lowest energy" pixel.
            // we need to define a range for out search. start by assuming it includes all three candidate indices
            int offsetStart = indexUpperLeft;
            int offsetEnd = indexUpperRight;
            if (!upperLeftIsIncluded)
            {
                // if no upper-left, we are on a left boundary. constrain start of range
                offsetStart = indexUpper;
            }
            
            if (!upperRightIsIncluded)
            {
                // if no upper-right, we're on a right boundary. constrain end of range
                offsetEnd = indexUpper;
            }
            //#ENDREGION
            
            // with the defined range to search across, look for the index of the next seam pixel
            auto seam_traceback_itr = std::find(cumulativeEnergyMap[i - 1].begin() + offsetStart, cumulativeEnergyMap[i - 1].begin() + offsetEnd, seam_traceback_value);
            int seam_traceback_index = std::distance(cumulativeEnergyMap[i - 1].begin(), seam_traceback_itr);

            // finally, assign this index as the next iterations seam pixel index
            seam_column_indices[i - 1] = seam_traceback_index;
        //#ENDREGION
        }
    }
}

/// @brief Transpose a given 2D vector.
/// @param imageMap 2D vector to transpose. Original is modified.
void transposeMap(vector<vector<int>> &imageMap)
{
    // the transpose map will need as many rows as imageMap has columns
    vector<vector<int>> transpose(imageMap[0].size(), vector<int>());

    for(int i = 0; i < imageMap.size(); ++i)
    {
        for(int j = 0; j < imageMap[i].size(); ++j)
        {   
            // for each pixel in this row of imageMap,
            // push into consecutive rows of the transpose

            // Ex)   
            // row: [1 2 3] of imageMap becomes 
            // 
            // column: [1]
            //         [2]
            //         [3]
 
            transpose[j].push_back(imageMap[i][j]);
        }
    }

    imageMap = transpose;

    return;
}

/// @brief Display a 2D vector.
/// @param map The 2D vector to be displayed. 
void displayMap(const vector<vector<int>> &map)
{
    for (const vector<int>& row : map)
    {
        for (int pixel : row)
        {
            if (pixel < 10)
            {
                cout << "  000" << pixel;
            }
            else if (pixel < 100)
            {
                cout << "  00" << pixel;
            }
            else if (pixel < 1000)
            {
                cout << "  0" << pixel;
            }
            else
            {
                cout << "  " << pixel;
            }
        }
        cout << "\n\n";
    }

    return;
}

/// @brief Helper. Utilize existing functions to display the transpose of a map 
///        without modifying the original.
/// @param map The 2D vector whose transpose is to be displayed.
void displayTranspose(const vector<vector<int>> &map)
{
    vector<vector<int>> temp(map);

    // transposes the temporary copy and displays it
    transposeMap(temp);
    displayMap(temp);

    return;
}

/// @brief Validate command-line args for the number of seams to remove are within the acceptable range.
/// @param imageMap The image map for which the seam carving requests are to be completed on.
/// @param num_vertical_seams Number of vertical seams to remove. If outside range [0, imageMap(num_columns) - 1], the request is invalid.
/// @param num_horizontal_seams Number of horizontal seams to remove. If outside range [0, imageMap(num_rows) - 1], the request is invalid.
void validateCarveRequests(const vector<vector<int>> &imageMap, int num_vertical_seams, int num_horizontal_seams)
{
    // validate vertical seam request
    if (num_vertical_seams < 0)
    {
        cerr << "error: requested number of vertical seams to carve must be greater than or equal to 0\n";
        exit(1);
    }
    if (num_vertical_seams >= imageMap[0].size())
    {
        cerr << "error: the requested number of vertical seams to carve is " << num_vertical_seams << ", which is invalid.\n"
             << "the provided image is " << imageMap[0].size() << " pixels wide. to carve the requested number of vertical seams would be to\n"
             << "erase the image entirely\n";
        exit(1);
    }

    // validate horizontal seam request
    if (num_horizontal_seams < 0)
    {
        cerr << "error: requested number of horizontal seams to carve must be greater than or equal to 0\n";
        exit(1);
    }
    if (num_horizontal_seams >= imageMap.size())
    {
        cerr << "error: the requested number of horizontal seams to carve is " << num_horizontal_seams << ", which is invalid.\n"
             << "the provided image is " << imageMap.size() << " pixels tall. to carve the requested number of horizontal seams would be to\n"
             << "erase the image entirely\n";
        exit(1);
    }

    return;
}

/// @brief  Write the seam-carved image map to a file. 
/// @param imageMap The image map that has been modified by the seam carving algorithm.
/// @param filename Name of the file to write the results to.
void writeResults(const vector<vector<int>> &imageMap, const string &filename)
{
    ofstream outFile(filename);

    outFile << "P2\n"; // for pgm file type
    outFile << "# Processed by Seam Carving Inc.\n"; // Seam Carving Incorporated!!!
    outFile << imageMap[0].size() << " " << imageMap.size() << "\n";  // first the # columns, then # rows, to match pgm file format for irfanview

    // we need to iterate through the imageMap and find the maximum value
    int max_val = imageMap[0][0];
    for (const auto& row : imageMap) 
    {
        for (auto val : row) 
        {
            if (val > max_val) 
            {
                max_val = val;
            }
        }
    }
    outFile << max_val << "\n";

    // write processed image map
    for (int i = 0; i < imageMap.size(); ++i)
    {
        for (int j = 0; j < imageMap[0].size(); ++j)
        {
            outFile << imageMap[i][j] << " ";
        }
        outFile << "\n";
    }

    return;
}
