#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <cctype>
#include <limits>

using namespace std;

/* -------------------- CONSTANTS -------------------- */
const int ROWS = 40;
const int COLS = 6;
const int SEATS_PER_TRIP = ROWS * COLS;

// Adjust if your dataset has larger passenger IDs
const int MAX_PASSENGER_ID = 1000000;

/* -------------------- DATA STRUCTURES -------------------- */
struct Passenger {
    int passengerID;
    int tripID;
    char name[50];
    int seatRow;      // internally stored as 0-39
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

    // seatRow (CSV is 1-40, convert to 0-39)
    if (!getline(ss, token, ',')) return false;
    int row;
    try {
        row = stoi(token);
    } catch (...) {
        return false;
    }

    if (row < 1 || row > 40) return false;
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

/* -------------------- CSV LOADING WITH OPTION 2 -------------------- */
void loadFromCSV(const string& filepath = "Flite_passenger_Dataset.csv") {
    auto start = chrono::high_resolution_clock::now();

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

        // Assign trip ID based on CSV row order (240 seats per trip)
        p.tripID = ((lineNumber - 1) / SEATS_PER_TRIP) + 1;

        // Validate seat row again (safety)
        if (!isValidSeatRow(p.seatRow))
            continue;

        // OPTION 2: If seat is taken, move to next available trip with same seat
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

    auto end = chrono::high_resolution_clock::now();
    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    cout << "\nCSV loaded successfully.\n";
    cout << "Passengers inserted: " << insertedCount << endl;
    cout << "Passengers moved to different trips: " << movedToNewTripCount << endl;
    cout << "Time taken: " << durationMs << " ms"
         << " (" << durationSec << " seconds)\n";
}

/* -------------------- DISPLAY MANIFEST BY TRIP -------------------- */
void displayManifestByTrip(int tripID) {
    Node* temp = head;
    bool found = false;

    cout << "\nPassenger Manifest for Trip " << tripID << "\n";
    cout << "-------------------------------------\n";

    while (temp) {
        if (temp->data.tripID == tripID) {
            found = true;
            cout << "ID: " << temp->data.passengerID
                 << " | Name: " << temp->data.name
                 << " | Seat: " << (temp->data.seatRow + 1) << temp->data.seatCol
                 << " | Class: " << temp->data.seatClass << endl;
        }
        temp = temp->next;
    }

    if (!found)
        cout << "No passengers found for this trip.\n";
}

/* -------------------- SEATING CHART -------------------- */
void displaySeatingForTrip(int tripID) {
    bool seats[ROWS][COLS] = { false };
    Node* temp = head;

    while (temp) {
        if (temp->data.tripID == tripID) {
            int r = temp->data.seatRow;
            int c = seatColToIndex(temp->data.seatCol);

            if (isValidSeatRow(r) && c >= 0 && c < COLS)
                seats[r][c] = true;
        }
        temp = temp->next;
    }

    cout << "\nSeating Chart for Trip " << tripID << "\n\n   ";
    for (int c = 0; c < COLS; c++)
        cout << char('A' + c) << " ";
    cout << endl;

    for (int r = 0; r < ROWS; r++) {
        cout << (r + 1 < 10 ? " " : "") << (r + 1) << " ";
        for (int c = 0; c < COLS; c++)
            cout << (seats[r][c] ? "X " : "O ");
        cout << endl;
    }
}

/* -------------------- ADD PASSENGER -------------------- */
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

    cout << "Enter Seat Row (1-40): ";
    int rowInput;
    if (!(cin >> rowInput)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    if (rowInput < 1 || rowInput > 40) {
        cout << "Seat row must be between 1 and 40.\n";
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

    appendPassenger(p);
    cout << "Passenger added successfully.\n";
}

/* -------------------- DELETE PASSENGER -------------------- */
void deletePassenger() {
    int id;
    cout << "\nEnter Passenger ID to delete: ";
    if (!(cin >> id)) {
        cout << "Invalid input.\n";
        clearInput();
        return;
    }

    Node* temp = head;
    Node* prev = nullptr;

    while (temp && temp->data.passengerID != id) {
        prev = temp;
        temp = temp->next;
    }

    if (!temp) {
        cout << "Passenger not found.\n";
        return;
    }

    if (!prev) {
        head = temp->next;
        if (!head) tail = nullptr;
    } else {
        prev->next = temp->next;
        if (temp == tail) tail = prev;
    }

    delete temp;
    cout << "Passenger removed successfully.\n";
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

    auto start = chrono::high_resolution_clock::now();
    Node* result = searchPassenger(id);
    auto end = chrono::high_resolution_clock::now();

    auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    double durationSec = durationMs / 1000.0;

    if (result) {
        cout << "\nPassenger Found!\n";
        cout << "Trip: " << result->data.tripID << endl;
        cout << "ID: " << result->data.passengerID << endl;
        cout << "Name: " << result->data.name << endl;
        cout << "Seat: " << (result->data.seatRow + 1) << result->data.seatCol << endl;
        cout << "Class: " << result->data.seatClass << endl;
    } else {
        cout << "\nPassenger not found.\n";
    }

    cout << "Search Time: " << durationMs << " ms"
         << " (" << durationSec << " seconds)\n";
}

/* -------------------- MAIN MENU -------------------- */
int main() {
    cout << "===== FLIGHT RESERVATION SYSTEM =====\n";

    // Automatically load the CSV file at startup
    cout << "\nLoading passenger data...\n";
    loadFromCSV("Flite_passenger_Dataset.csv");

    int choice;
    do {
        cout << "\n===== MAIN MENU =====\n";
        cout << "1. Add Passenger\n";
        cout << "2. Delete Passenger\n";
        cout << "3. Display Manifest By Trip\n";
        cout << "4. Display Seating Chart (By Trip)\n";
        cout << "5. Search Passenger By ID\n";
        cout << "0. Exit\n";
        cout << "Choice: ";

        if (!(cin >> choice)) {
            cout << "Invalid input.\n";
            clearInput();
            continue;
        }

        auto start = chrono::high_resolution_clock::now();

        switch (choice) {
        case 1:
            insertPassenger();
            break;

        case 2:
            deletePassenger();
            break;

        case 3: {
            int trip;
            cout << "Enter Trip Number: ";
            if (!(cin >> trip)) {
                cout << "Invalid input.\n";
                clearInput();
                break;
            }
            displayManifestByTrip(trip);
            break;
        }

        case 4: {
            int trip;
            cout << "Enter Trip Number: ";
            if (!(cin >> trip)) {
                cout << "Invalid input.\n";
                clearInput();
                break;
            }
            displaySeatingForTrip(trip);
            break;
        }

        case 5:
            searchPassengerByID();
            break;

        case 0:
            cout << "Goodbye!\n";
            break;

        default:
            cout << "Invalid choice.\n";
        }

        auto end = chrono::high_resolution_clock::now();
        auto durationMs = chrono::duration_cast<chrono::milliseconds>(end - start).count();
        double durationSec = durationMs / 1000.0;

        if (choice != 0) {
            cout << "Operation Time: " << durationMs << " ms"
                 << " (" << durationSec << " seconds)\n";
        }

    } while (choice != 0);

    // Cleanup memory
    freeList();
    return 0;
}
