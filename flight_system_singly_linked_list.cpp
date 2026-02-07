#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <cctype>
#include <limits>
#include <iomanip>

using namespace std;

/* -------------------- CONSTANTS -------------------- */
const int ROWS = 30;  // Changed from 40 to 30
const int COLS = 6;
const int SEATS_PER_TRIP = ROWS * COLS;  // Now 180 seats per trip

// Adjust if your dataset has larger passenger IDs
const int MAX_PASSENGER_ID = 1000000;

/* -------------------- MEMORY TRACKING -------------------- */
size_t dynamicMemory = 0;  // Track dynamic memory allocation

/* -------------------- DATA STRUCTURES -------------------- */
struct Passenger {
    int passengerID;
    int tripID;
    char name[50];
    int seatRow;      // internally stored as 0-29
    char seatCol;     // 'A' - 'F'
    char seatClass[10];
};

struct Node {
    Passenger data;
    Node* next;
};

Node* head = nullptr;
Node* tail = nullptr;   // Tail pointer for O(1) append

/* -------------------- HELPER FUNCTIONS -------------------- */
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

bool isValidSeatRow(int row) {
    return row >= 0 && row < ROWS;
}

bool isValidSeatCol(char col) {
    col = toupper(col);
    return col >= 'A' && col <= 'F';
}

bool isValidTripID(int tripID) {
    return tripID >= 1;
}

bool isValidPassengerID(int id) {
    return id > 0;
}

bool isValidSeatClass(const char seatClass[]) {
    return (strcmp(seatClass, "Economy") == 0 ||
            strcmp(seatClass, "Business") == 0 ||
            strcmp(seatClass, "First") == 0);
}

int seatColToIndex(char col) {
    return toupper(col) - 'A';
}

/* -------------------- MEMORY USAGE DISPLAY -------------------- */
void displayMemoryUsage() {
    // Static memory calculation
    size_t staticMemory = 0;
    
    // Global variables
    staticMemory += sizeof(head);
    staticMemory += sizeof(tail);
    staticMemory += sizeof(dynamicMemory);
    
    // Constants
    staticMemory += sizeof(ROWS);
    staticMemory += sizeof(COLS);
    staticMemory += sizeof(SEATS_PER_TRIP);
    staticMemory += sizeof(MAX_PASSENGER_ID);
    
    // Static array for duplicate checking (used in loadFromCSV)
    staticMemory += sizeof(bool) * (MAX_PASSENGER_ID + 1);
    
    size_t totalMemory = staticMemory + dynamicMemory;
    
    cout << "\n===== TOTAL MEMORY USAGE =====\n";
    cout << "Static memory (arrays + structs)  : " << staticMemory << " bytes\n";
    cout << "Dynamic memory (all strings)       : " << dynamicMemory << " bytes\n";
    cout << "Total approximate memory usage     : " << totalMemory << " bytes";
    cout << " (" << fixed << setprecision(2) << (totalMemory / 1024.0) << " KB, ";
    cout << (totalMemory / (1024.0 * 1024.0)) << " MB)\n";
}

/* -------------------- LINKED LIST FUNCTIONS -------------------- */
Node* searchPassenger(int id) {
    Node* temp = head;
    while (temp) {
        if (temp->data.passengerID == id)
            return temp;
        temp = temp->next;
    }
    return nullptr;
}

bool isSeatTaken(int tripID, int seatRow, char seatCol) {
    Node* temp = head;
    while (temp) {
        if (temp->data.tripID == tripID &&
            temp->data.seatRow == seatRow &&
            toupper(temp->data.seatCol) == toupper(seatCol)) {
            return true;
        }
        temp = temp->next;
    }
    return false;
}

void appendPassenger(const Passenger& p) {
    Node* newNode = new Node;
    newNode->data = p;
    newNode->next = nullptr;
    
    // Track dynamic memory
    dynamicMemory += sizeof(Node);

    if (!head) {
        head = tail = newNode;
    } else {
        tail->next = newNode;
        tail = newNode;
    }
}

void freeList() {
    Node* temp = head;
    while (temp) {
        Node* nextNode = temp->next;
        delete temp;
        temp = nextNode;
    }
    head = nullptr;
    tail = nullptr;
    dynamicMemory = 0;
}

/* -------------------- CSV PARSING -------------------- */
bool parseCSVLine(const string& line, Passenger& p) {
    string token;
    stringstream ss(line);

    // passengerID
    if (!getline(ss, token, ',')) return false;
    try {
        p.passengerID = stoi(token);
    } catch (...) {
        return false;
    }

    // name
    if (!getline(ss, token, ',')) return false;
    strncpy(p.name, token.c_str(), sizeof(p.name) - 1);
    p.name[sizeof(p.name) - 1] = '\0';

    // seatRow (CSV is 1-30, convert to 0-29)
    if (!getline(ss, token, ',')) return false;
    int row;
    try {
        row = stoi(token);
    } catch (...) {
        return false;
    }

    if (row < 1 || row > 30) return false;
    p.seatRow = row - 1;

    // seatCol (A-F)
    if (!getline(ss, token, ',')) return false;
    if (token.empty()) return false;
    p.seatCol = toupper(token[0]);
    if (!isValidSeatCol(p.seatCol)) return false;

    // seatClass
    if (!getline(ss, token, ',')) return false;
    strncpy(p.seatClass, token.c_str(), sizeof(p.seatClass) - 1);
    p.seatClass[sizeof(p.seatClass) - 1] = '\0';

    return true;
}

/* -------------------- CSV LOADING -------------------- */
void loadFromCSV(const string& filepath = "Flite_passenger_Dataset.csv") {
    string path = filepath;

    // If no filepath provided, ask user
    if (path.empty()) {
        cout << "Enter CSV file path (or just filename if in same folder): ";
        getline(cin, path);
    }

    ifstream file(path);
    if (!file) {
        cout << "CSV file not found. Please check the path.\n";
        return;
    }

    // START TIMING - after file is opened successfully
    auto start = chrono::high_resolution_clock::now();

    // Faster duplicate checking for passenger IDs
    static bool seenID[MAX_PASSENGER_ID + 1] = { false };

    string line;
    getline(file, line); // Skip header

    int insertedCount = 0;
    int lineNumber = 0;
    int movedToNewTripCount = 0;

    while (getline(file, line)) {
        lineNumber++;

        Passenger p;

        if (!parseCSVLine(line, p))
            continue;

        if (!isValidPassengerID(p.passengerID))
            continue;

        // Duplicate check (fast)
        if (p.passengerID <= MAX_PASSENGER_ID && seenID[p.passengerID])
            continue;

        // Assign trip ID based on CSV row order (180 seats per trip)
        p.tripID = ((lineNumber - 1) / SEATS_PER_TRIP) + 1;

        // Validate seat row again (safety)
        if (!isValidSeatRow(p.seatRow))
            continue;

        // If seat is taken, move to next available trip with same seat
        int originalTripID = p.tripID;
        while (isSeatTaken(p.tripID, p.seatRow, p.seatCol)) {
            p.tripID++;
        }

        if (p.tripID != originalTripID) {
            movedToNewTripCount++;
        }

        appendPassenger(p);

        if (p.passengerID <= MAX_PASSENGER_ID)
            seenID[p.passengerID] = true;

        insertedCount++;
    }

    file.close();

    // END TIMING - after all processing is complete
    auto end = chrono::high_resolution_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    cout << "\nCSV loaded successfully.\n";
    cout << "Passengers inserted: " << insertedCount << endl;
    cout << "Passengers moved to different trips: " << movedToNewTripCount << endl;
    cout << "Time taken: " << durationMs << " ms"
         << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
}

/* -------------------- DISPLAY MANIFEST BY TRIP -------------------- */
void displayManifestByTrip(int tripID) {
    // START TIMING - after input is received
    auto start = chrono::high_resolution_clock::now();

    Node* temp = head;
    bool found = false;
    int passengerCount = 0;

    cout << "\n--- Passenger Manifest for Trip " << tripID << " ---\n";

    while (temp) {
        if (temp->data.tripID == tripID) {
            found = true;
            passengerCount++;
            cout << "Passenger " << passengerCount << ": " << temp->data.passengerID 
                 << " | " << temp->data.name
                 << " | Row " << (temp->data.seatRow + 1) << " Seat " << temp->data.seatCol
                 << " | " << temp->data.seatClass << endl;
        }
        temp = temp->next;
    }

    if (!found)
        cout << "No passengers found for Trip " << tripID << ".\n";

    // END TIMING
    auto end = chrono::high_resolution_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    cout << "\nOperation Time: " << durationMs << " ms"
         << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
}

/* -------------------- DISPLAY SEATING CHART -------------------- */
void displaySeatingForTrip(int tripID) {
    // START TIMING - after input is received
    auto start = chrono::high_resolution_clock::now();

    bool seats[ROWS][COLS] = { false };
    Passenger* seatData[ROWS][COLS] = { nullptr };

    Node* temp = head;
    while (temp) {
        if (temp->data.tripID == tripID) {
            int r = temp->data.seatRow;
            int c = seatColToIndex(temp->data.seatCol);
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
                seats[r][c] = true;
                seatData[r][c] = &(temp->data);
            }
        }
        temp = temp->next;
    }

    cout << "\n--- Seating Chart for Trip " << tripID << " ---\n";
    cout << "    A       B       C       D       E       F\n";

    for (int r = 0; r < ROWS; r++) {
        cout << "R" << (r + 1) << ": ";
        for (int c = 0; c < COLS; c++) {
            if (seats[r][c] && seatData[r][c] != nullptr) {
                cout << seatData[r][c]->passengerID << " | " 
                     << seatData[r][c]->name << " | Seat " 
                     << char('A' + c) << " | " 
                     << seatData[r][c]->seatClass;
                if (c < COLS - 1) cout << " ";
                break; // Only show first passenger per row for cleaner display
            }
        }
        cout << endl;
    }

    // END TIMING
    auto end = chrono::high_resolution_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    cout << "\nOperation Time: " << durationMs << " ms"
         << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
}

/* -------------------- LIST PASSENGERS BY CLASS -------------------- */
void listPassengersByClass() {
    int tripID;
    cout << "\nEnter Trip Number: ";
    if (!(cin >> tripID)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    char classType[10];
    cout << "Enter Class (Economy/Business/First): ";
    cin >> classType;

    // START TIMING - after all inputs are collected
    auto start = chrono::high_resolution_clock::now();

    Node* temp = head;
    bool found = false;
    int count = 0;

    cout << "\nPassengers in " << classType << " class for Trip " << tripID << ":\n";
    cout << "-------------------------------------\n";

    while (temp) {
        if (temp->data.tripID == tripID && strcmp(temp->data.seatClass, classType) == 0) {
            found = true;
            count++;
            cout << count << ". ID: " << temp->data.passengerID
                 << " | Name: " << temp->data.name
                 << " | Seat: Row " << (temp->data.seatRow + 1) << " Seat " << temp->data.seatCol << endl;
        }
        temp = temp->next;
    }

    if (!found)
        cout << "No passengers found in this class for this trip.\n";

    // END TIMING
    auto end = chrono::high_resolution_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    cout << "\nOperation Time: " << durationMs << " ms"
         << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
}

/* -------------------- ADD PASSENGER (MAKE RESERVATION) -------------------- */
void insertPassenger() {
    Passenger p;

    cout << "\nEnter Passenger ID: ";
    if (!(cin >> p.passengerID)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    if (!isValidPassengerID(p.passengerID)) {
        cout << "Passenger ID must be positive.\n";
        return;
    }

    if (searchPassenger(p.passengerID)) {
        cout << "Passenger ID already exists!\n";
        return;
    }

    cout << "Enter Trip Number (>=1): ";
    if (!(cin >> p.tripID)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    if (!isValidTripID(p.tripID)) {
        cout << "Trip number must be 1 or above.\n";
        return;
    }

    cout << "Enter Name: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.getline(p.name, 50);

    cout << "Enter Seat Row (1-30): ";
    int rowInput;
    if (!(cin >> rowInput)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    if (rowInput < 1 || rowInput > 30) {
        cout << "Seat row must be between 1 and 30.\n";
        return;
    }
    p.seatRow = rowInput - 1;

    cout << "Enter Seat Column (A-F): ";
    cin >> p.seatCol;
    p.seatCol = toupper(p.seatCol);

    if (!isValidSeatCol(p.seatCol)) {
        cout << "Seat column must be between A and F.\n";
        return;
    }

    if (isSeatTaken(p.tripID, p.seatRow, p.seatCol)) {
        cout << "This seat is already taken for this trip.\n";
        return;
    }

    cout << "Enter Class (Economy/Business/First): ";
    cin >> p.seatClass;

    if (!isValidSeatClass(p.seatClass)) {
        cout << "Invalid class. Please enter Economy, Business, or First.\n";
        return;
    }

    // START TIMING - after all inputs are validated and collected
    auto start = chrono::high_resolution_clock::now();

    appendPassenger(p);

    // END TIMING
    auto end = chrono::high_resolution_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    cout << "Reservation made successfully.\n";
    cout << "\nOperation Time: " << durationMs << " ms"
         << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
}

/* -------------------- DELETE PASSENGER (CANCEL RESERVATION) -------------------- */
void deletePassenger() {
    int id;
    cout << "\nEnter Passenger ID to cancel reservation: ";
    if (!(cin >> id)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    // START TIMING - after input is collected
    auto start = chrono::high_resolution_clock::now();

    Node* temp = head;
    Node* prev = nullptr;

    while (temp && temp->data.passengerID != id) {
        prev = temp;
        temp = temp->next;
    }

    if (!temp) {
        auto end = chrono::high_resolution_clock::now();
        auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        double durationSec = durationMs / 1000.0;

        cout << "Passenger not found.\n";
        cout << "\nOperation Time: " << durationMs << " ms"
             << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
        return;
    }

    if (!prev) {
        head = temp->next;
        if (!head) tail = nullptr;
    } else {
        prev->next = temp->next;
        if (temp == tail) tail = prev;
    }

    // Update dynamic memory tracking
    dynamicMemory -= sizeof(Node);

    delete temp;

    // END TIMING
    auto end = chrono::high_resolution_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    cout << "Reservation cancelled successfully.\n";
    cout << "\nOperation Time: " << durationMs << " ms"
         << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
}

/* -------------------- SEARCH PASSENGER -------------------- */
void searchPassengerByID() {
    int id;
    cout << "\nEnter Passenger ID to search: ";
    if (!(cin >> id)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    // START TIMING - after input is collected, before search begins
    auto start = chrono::high_resolution_clock::now();
    Node* result = searchPassenger(id);
    auto end = chrono::high_resolution_clock::now();
    // END TIMING

    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    if (result) {
        cout << "\nPassenger Found!\n";
        cout << "Trip: " << result->data.tripID << endl;
        cout << "ID: " << result->data.passengerID << endl;
        cout << "Name: " << result->data.name << endl;
        cout << "Seat: Row " << (result->data.seatRow + 1) << " Seat " << result->data.seatCol << endl;
        cout << "Class: " << result->data.seatClass << endl;
    } else {
        cout << "\nPassenger not found.\n";
    }

    cout << "\nSearch Time: " << durationMs << " ms"
         << " (" << fixed << setprecision(3) << durationSec << " seconds)\n";
}

/* -------------------- MAIN MENU -------------------- */
int main() {
    cout << "===== FLIGHT RESERVATION SYSTEM =====\n";

    // Automatically load the CSV file at startup
    cout << "\nLoading passenger data...\n";
    loadFromCSV("Flite_passenger_Dataset.csv");

    int choice;
    do {
        cout << "\n===== FLIGHT RESERVATION SYSTEM =====\n";
        cout << "1. View Seating Chart\n";
        cout << "2. View Passenger Manifest (By Trip)\n";
        cout << "3. Search Passenger\n";
        cout << "4. Cancel Reservation\n";
        cout << "5. List Passengers by Class\n";
        cout << "6. Make a Reservation\n";
        cout << "0. Exit\n";
        cout << "Enter choice: ";

        if (!(cin >> choice)) {
            cout << "Invalid input.\n";
            clearInput();
            continue;
        }

        switch (choice) {
        case 1: {
            int trip;
            cout << "Enter Trip Number (1-56): ";
            if (!(cin >> trip)) {
                cout << "Invalid input.\n";
                clearInput();
                break;
            }
            displaySeatingForTrip(trip);
            break;
        }

        case 2: {
            int trip;
            cout << "Enter Trip Number (1-56): ";
            if (!(cin >> trip)) {
                cout << "Invalid input.\n";
                clearInput();
                break;
            }
            displayManifestByTrip(trip);
            break;
        }

        case 3:
            searchPassengerByID();
            break;

        case 4:
            deletePassenger();
            break;

        case 5:
            listPassengersByClass();
            break;

        case 6:
            insertPassenger();
            break;

        case 0:
            cout << "Goodbye!\n";
            break;

        default:
            cout << "Invalid choice.\n";
        }

        // Display memory usage after each operation (except exit)
        if (choice != 0 && choice >= 1 && choice <= 6) {
            displayMemoryUsage();
        }

    } while (choice != 0);

    // Cleanup memory
    freeList();
    return 0;
}
