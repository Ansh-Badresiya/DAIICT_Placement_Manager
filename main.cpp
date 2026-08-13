// ============================================================
//  main.cpp  —  Entry point for the Optimized Placement Manager
//
//  Menu structure is identical to the original project.
//  The PlacementManager class is properly #include'd via its
//  header (not via #include of .cpp as in the original).
// ============================================================

#include <iostream>
#include <string>
#include "../include/PlacementManager.h"

using namespace std;

// -------------------------------------------------------
void displayMainMenu()
{
    cout << "\n\n<-----------------------------------------> Placement Management System Menu <--------------------------------------->\n";
    cout << "\n\n-------> Please select an option from the menu below:\n";
    cout << "\n1. Input Placement Data\n";
    cout << "2. Sort Data\n";
    cout << "3. View Placement Statistics\n";
    cout << "4. View Not Selected Students\n";
    cout << "5. Exit\n";
    cout << "\n<---------------------------------------------------------------------------------------------------------------------->\n";
    cout << "\nEnter your choice: ";
}

// -------------------------------------------------------
void displaySortMenu(PlacementManager& p)
{
    cout << "\n\n<----------------------------------------------------> Sort Data Menu <------------------------------------------------->\n";
    cout << "\n\n-------> Please select an option from the menu below:\n";
    cout << "\n1.  Sort Whole Data\n";
    cout << "2.  Sort Data Batch Wise\n";
    cout << "3.  Sort Data Program Wise\n";
    cout << "4.  Sort Data Year Wise\n";
    cout << "5.  Sort Data Company Wise\n";
    cout << "6.  Sort Data Batch and Company Wise\n";
    cout << "7.  Sort Data Program of Batch Wise\n";
    cout << "8.  Sort Data Program of Company Wise\n";
    cout << "9.  Sort Data Program of Year Wise\n";
    cout << "10. Sort Data Year and Batch Wise\n";
    cout << "\n<---------------------------------------------------------------------------------------------------------------------->\n";
    cout << "\nEnter your choice: ";

    int choice; cin >> choice;
    switch (choice) {
    case 1:  p.SortWholeData();               break;
    case 2:  p.SortDataBatchWise();           break;
    case 3:  p.SortDataProgramWise();         break;
    case 4:  p.SortDataYearWise();            break;
    case 5:  p.SortDataCompanyWise();         break;
    case 6:  p.SortDataBatchAndCompanyWise(); break;
    case 7:  p.SortDataProgramOFBatchWise();  break;
    case 8:  p.SortDataProgramOFCompanyWise();break;
    case 9:  p.SortDataProgramOFYearWise();   break;
    case 10: p.SortDataYearAndBatchWise();    break;
    default: cout << "\n<---- Invalid choice! ---->\n\n"; break;
    }
}

// -------------------------------------------------------
void displayStatisticsMenu(PlacementManager& p)
{
    cout << "\n\n<----------------------------------------------> View Placement Statistics Menu <---------------------------------------->\n";
    cout << "\n\n-------> Please select an option from the menu below:\n";
    cout << "\n1.  Overall Placement Statistics\n";
    cout << "2.  Student Placement Details\n";
    cout << "3.  Batch Wise Placement Statistics\n";
    cout << "4.  Program Wise Placement Statistics\n";
    cout << "5.  Company Wise Placement Statistics\n";
    cout << "6.  Year Wise Placement Statistics\n";
    cout << "7.  Batch and Company Wise Placement Statistics\n";
    cout << "8.  Program and Batch Wise Placement Statistics\n";
    cout << "9.  Program and Company Wise Placement Statistics\n";
    cout << "10. Year and Batch Wise Placement Statistics\n";
    cout << "11. Year and Company Wise Placement Statistics\n";
    cout << "12. Year and Program Wise Placement Statistics\n";
    cout << "\n<---------------------------------------------------------------------------------------------------------------------->\n";
    cout << "\nEnter your choice: ";

    int choice; cin >> choice;
    switch (choice) {
    case 1:  p.FindOverallPlacementStatistics();              break;
    case 2:  p.FindStudentPlacementDetails();                 break;
    case 3:  p.FindBatchWisePlacementStatistics();            break;
    case 4:  p.FindProgramWisePlacementStatistics();          break;
    case 5:  p.FindCompanyWisePlacementStatistics();          break;
    case 6:  p.FindYearWisePlacementStatistics();             break;
    case 7:  p.FindBatchAndCompanyWisePlacementStatistics();  break;
    case 8:  p.FindProgramAndBatchWisePlacementStatistics();  break;
    case 9:  p.FindProgramAndCompanyWisePlacementStatistics();break;
    case 10: p.FindYearAndBatchWisePlacementStatistics();     break;
    case 11: p.FindYearAndCompanyWisePlacementStatistics();   break;
    case 12: p.FindYearAndProgramWisePlacementStatistics();   break;
    default: cout << "\n<---- Invalid choice! ---->\n\n"; break;
    }
}

// -------------------------------------------------------
void displayNotSelectedMenu(PlacementManager& p)
{
    cout << "\n\n<---------------------------------------------> View Not Selected Students Menu <--------------------------------------->\n";
    cout << "\n\n-------> Please select an option from the menu below:\n";
    cout << "\n1.  Not Selected Batch Wise\n";
    cout << "2.  Not Selected Program Wise\n";
    cout << "3.  Not Selected Company Wise\n";
    cout << "4.  Not Selected Year Wise\n";
    cout << "5.  Not Selected Batch and Program Wise\n";
    cout << "6.  Not Selected Batch and Company Wise\n";
    cout << "7.  Not Selected Company and Program Wise\n";
    cout << "8.  Not Selected Year and Batch Wise\n";
    cout << "9.  Not Selected Year and Program Wise\n";
    cout << "10. Not Selected Year and Company Wise\n";
    cout << "\n<---------------------------------------------------------------------------------------------------------------------->\n";
    cout << "\nEnter your choice: ";

    int choice; cin >> choice;
    switch (choice) {
    case 1:  p.FindNotSelectedBatchWise();           break;
    case 2:  p.FindNotSelectedProgramWise();         break;
    case 3:  p.FindNotSelectedCompanyWise();         break;
    case 4:  p.FindNotSelectedYearWise();            break;
    case 5:  p.FindNotSelectedBatchAndProgramWise(); break;
    case 6:  p.FindNotSelectedBatchAndCompanyWise(); break;
    case 7:  p.FindNotSelectedCompanyAndProgramWise();break;
    case 8:  p.FindNotSelectedYearAndBatchWise();    break;
    case 9:  p.FindNotSelectedYearAndProgramWise();  break;
    case 10: p.FindNotSelectedYearAndCompanyWise();  break;
    default: cout << "\n<---- Invalid choice! ---->\n\n"; break;
    }
}

// -------------------------------------------------------
int main()
{
    PlacementManager p;
    int choice;

    cout << "\n**********************************************************************************************************************\n";
    cout << "****************************************                                     *****************************************\n";
    cout << "**************************************         Welcome to the Placement       ****************************************\n";
    cout << "*************************************             Management System!           ***************************************\n";
    cout << "**************************************                                        ****************************************\n";
    cout << "****************************************                                     *****************************************\n";
    cout << "**********************************************************************************************************************\n";

    do {
        displayMainMenu();
        cin >> choice;

        switch (choice) {
        case 1:
            cout << "\n<-------x-------x-------x-------x-------x-------x-------x-------x-------x-------x------>\n\n";
            p.InputPlacementData();
            cout << "\n<-------x-------x-------x-------x-------x-------x-------x-------x-------x-------x------>\n";
            break;

        case 2:
            cout << "\n<-------x-------x-------x-------x-------x-------x-------x-------x-------x-------x------>\n\n";
            displaySortMenu(p);
            break;

        case 3:
            cout << "\n<-------x-------x-------x-------x-------x-------x-------x-------x-------x-------x------>\n\n";
            displayStatisticsMenu(p);
            break;

        case 4:
            cout << "\n<-------x-------x-------x-------x-------x-------x-------x-------x-------x-------x------>\n\n";
            displayNotSelectedMenu(p);
            break;

        case 5:
            cout << "\n          <-------x-------x-------x-------x-------x-------x-------x-------x-------x-------x------>\n";
            cout << "\n<------------------------------------------------- Exiting. Goodbye! -------------------------------------->\n";
            cout << "\n          <-------x-------x-------x-------x-------x-------x-------x-------x-------x-------x------>\n\n";
            break;

        default:
            cout << "\n<---- Invalid choice! ---->\n\n";
        }

    } while (choice != 5);

    return 0;
}
