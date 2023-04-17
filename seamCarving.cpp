/* 
    seamCarving.cpp

    Seam carving changes the size of an image by removing the least visible pixels in the image. 
    The visibility of a pixel can be defined using an energy function. Seam carving can be done by finding a 
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
using std::vector;
using std::string;
using std::stringstream;

vector<vector<int>> initImageMap(const string &filename);
vector<vector<int>> initEnergyMap(const vector<vector<int>> &imageMap);
vector<vector<int>> initCumulativeEnergyMap(const vector<vector<int>> &energyMap);
void seamCarver(vector<vector<int>> &imageMap, const vector<vector<int>> &cumulativeEnergyMap);
void displayMap(const vector<vector<int>> &map);

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

    cout << "Image Map For '" << argv[1] << "': \n";
    displayMap(I);

    // INITIALIZE THE ENERGY MAP
    vector<vector<int>> E = initEnergyMap(I);
    
    cout << "\nEnergy Map: \n";
    displayMap(E);

    // INITIALIZE THE CUMULATIVE ENERGY MAP 
    vector<vector<int>> CE = initCumulativeEnergyMap(E);

    cout << "\nCumulative Energy Map: \n";
    displayMap(CE);

    // CARVE OUT A SEAM
    seamCarver(I, CE); 

    cout << "\nSeam-Carved Image: \n";
    displayMap(I);
   
    return 0;
}

// <Summary> a 2D vector of integers is populated with the image pixel values comprising the pgm image file, 'filename' </Summary> 
// <Param name='filename'> name of a file with a .pgm extension </Param> 
// <Return> the resultant image map by value </Return> 
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

    // #REGION parse_to_data 
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

// <Summary> a 2D vector of integers is populated with pixel energy values given an image map produced by initImageMap </Summary> 
// <Param name='imageMap'> a 2D vector containing the pixel data of a pgm file </Param> 
// <Return> the resultant energy map by value </Return> 
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

// <Summary>
//      a 2D vector of integers is populated with cumulative energy values to provide the seam 
//      carving algorithm with information about the lowest energy seam 
// </Summary> 
// <Param name='energyMap'> the energy map which is used to derive the CE map </Param> 
// <Return> 
//      the resultant cumulative energy map by value
//      the lowest energy value in the final row of the CE matrix represents the pixel 
//      which ends the lowest energy seam
// </Return> 
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
            vector<int> validSeamOriginList;

            // validate bounds of upper-left
            if ((j - 1) >= 0) 
            { 
                validSeamOriginList.push_back(result[i - 1][j - 1]);
            }

            // no need to validate upper
            validSeamOriginList.push_back(result[i - 1][j]);

            // validate bounds of upper-right
            if ((j + 1) < energyMap[i].size()) 
            { 
                validSeamOriginList.push_back(result[i - 1][j + 1]);
            }
            //#ENDREGION

            // at this pixel, add to its energy the minimum energy contained within validSeamOriginList
            result[i][j] += *std::min_element(validSeamOriginList.begin(), validSeamOriginList.end());
        }
    }

    return result;
}

// <Summary>
//      given the image map and its cumulative energy map, "carve out" the lowest energy seam 
//      from the image map
// </Summary> 
// <Param name='imageMap'> the image map to be modified by the seamCarver </Param> 
// <Param name='cumulativeEnergyMap'> the CE map to be traced back to determine the lowest energy seam </Param> 
// <Return> N/A -> the seam-carved image map is modified in-place </Return> 
void seamCarver(vector<vector<int>> &imageMap, const vector<vector<int>> &cumulativeEnergyMap)
{
    // modify imageMap by identifying the pixel in each row that is an element of the 
    // lowest energy seam, shifting it the end of the row, and popping back that element

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
    // and next trace-back the seam to find the connecting seam pixel below it for the next iteration
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
        
        //#REGION find out what the next seam pixel index is
        if ((i - 1) >= 0)
        {
            // if-block only reached if trace-back step doesn't take us out of bounds

            //#REGION bounds checking
            // create a list of 3 (or 2) seam origin pixels in the row above that can be connected
            // to the seam pixel in the current row.
            vector<int> validSeamOriginList; 

            // validate bounds of upper-left
            if ((seam_column_indices[i] - 1) >= 0) 
            { 
                validSeamOriginList.push_back(cumulativeEnergyMap[i - 1][seam_column_indices[i] - 1]);
            }

            // no need to validate upper
            validSeamOriginList.push_back(cumulativeEnergyMap[i - 1][seam_column_indices[i]]);

            // validate bounds of upper-right
            if ((seam_column_indices[i] + 1) < cumulativeEnergyMap[i].size()) 
            { 
                validSeamOriginList.push_back(cumulativeEnergyMap[i - 1][seam_column_indices[i] + 1]);
            }
            //#ENDREGION

            // among the possible seam trace-back pixels in the list, find the lowest energy
            int seam_traceback_value = *std::min_element(validSeamOriginList.begin(), validSeamOriginList.end()); 
            
            // find the index in the trace-back row of this "lowest energy" pixel
            auto seam_traceback_itr = std::find(cumulativeEnergyMap[i - 1].begin(), cumulativeEnergyMap[i - 1].end(), seam_traceback_value);
            int seam_traceback_index = std::distance(cumulativeEnergyMap[i - 1].begin(), seam_traceback_itr);

            // finally, assign this index as the next iterations seam pixel index
            seam_column_indices[i - 1] = seam_traceback_index;
        //#ENDREGION
        }
    }
}

// <Summary> display a 2D vector </Summary> 
// <Param name='map'> a 2D vector to be displayed </Param> 
// <Return> N/A </Return> 
void displayMap(const vector<vector<int>> &map)
{
    for (vector<int> row : map)
    {
        for (int pixel : row)
        {
            if (pixel < 100 && pixel >= 10)
            {
                cout << " 0" << pixel << " ";
            }
            else if (pixel < 10)
            {
                cout <<  " 00" << pixel << " ";
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
