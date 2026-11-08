#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <limits>

using namespace std;

// ==========================================
// CUSTOM EXCEPTION CLASSES
// ==========================================
class FleetException : public runtime_error {
public:
    explicit FleetException(const string& message) : runtime_error(message) {}
};

class FileOperationException : public FleetException {
public:
    explicit FileOperationException(const string& message) : FleetException(message) {}
};

class InvalidCsvFormatException : public FleetException {
public:
    explicit InvalidCsvFormatException(const string& message) : FleetException(message) {}
};

class DuplicateVehicleException : public FleetException {
public:
    explicit DuplicateVehicleException(const string& message) : FleetException(message) {}
};

class VehicleNotFoundException : public FleetException {
public:
    explicit VehicleNotFoundException(const string& message) : FleetException(message) {}
};

class InvalidVehicleException : public FleetException {
public:
    explicit InvalidVehicleException(const string& message) : FleetException(message) {}
};

// ==========================================
// CLASS VEHICLE (SERIALISASI & DESERIALISASI)
// ==========================================
class Vehicle {
private:
    string id, type, brand, status;
    int year, capacity;

public:
    Vehicle() : id(""), type(""), brand(""), year(0), capacity(0), status("") {}
    Vehicle(string id, string type, string brand, int year, int capacity, string status)
        : id(id), type(type), brand(brand), year(year), capacity(capacity), status(status) {}

    string getId() const { return id; }
    string getType() const { return type; }
    string getBrand() const { return brand; }
    int getYear() const noexcept { return year; }
    int getCapacity() const { return capacity; }
    string getStatus() const { return status; }

    void display() const {
        cout << id << " | " << type << " | " << brand << " | "
             << year << " | " << capacity << " | " << status << '\n';
    }

    bool isValid() const {
        bool textOk = id.find_first_not_of(" \t\r\n") != string::npos &&
                      brand.find_first_not_of(" \t\r\n") != string::npos;
        bool csvOk = (id + brand).find_first_of(",\"\r\n") == string::npos;
        bool typeOk = type == "Car" || type == "Truck" || type == "Bus";
        bool statusOk = status == "ACTIVE" || status == "MAINTENANCE" || status == "INACTIVE";
        return textOk && csvOk && typeOk && statusOk &&
               year >= 1886 && year <= 2100 && capacity > 0;
    }

    // Serialisasi: Objek -> Row CSV
    string toCsvRow() const {
        return id + "," + type + "," + brand + "," + to_string(year) + "," +
               to_string(capacity) + "," + status;
    }

    // Deserialisasi: Row CSV -> Objek
    static Vehicle fromCsvRow(const string& row) {
        string line = row;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        istringstream ss(line);
        string id, type, brand, yearStr, capStr, status;

        if (!getline(ss, id, ',') || !getline(ss, type, ',') ||
            !getline(ss, brand, ',') || !getline(ss, yearStr, ',') ||
            !getline(ss, capStr, ',') || !getline(ss, status) ||
            status.find(',') != string::npos) {
            throw InvalidCsvFormatException("Jumlah kolom CSV tidak valid atau mengandung pemisah ilegal.");
        }

        int year = 0, capacity = 0;
        try {
            size_t idx1 = 0, idx2 = 0;
            year = stoi(yearStr, &idx1);
            capacity = stoi(capStr, &idx2);
            if (idx1 != yearStr.length() || idx2 != capStr.length()) {
                throw InvalidCsvFormatException("Format numerik tahun/kapasitas memuat karakter non-angka.");
            }
        } catch (const invalid_argument&) {
            throw InvalidCsvFormatException("Gagal mengonversi tahun/kapasitas ke angka (invalid_argument).");
        } catch (const out_of_range&) {
            throw InvalidCsvFormatException("Nilai tahun/kapasitas melampaui batas integer.");
        }

        Vehicle v(id, type, brand, year, capacity, status);
        if (!v.isValid()) {
            throw InvalidCsvFormatException("Atribut kendaraan dari CSV tidak memenuhi aturan validasi data.");
        }
        return v;
    }
};

// ==========================================
// CLASS FLEET (MANAGEMENT & TRANSAKSIONAL I/O)
// ==========================================
class Fleet {
private:
    vector<Vehicle> vehicles;

public:
    int find(string id) const {
        for (int i = 0; i < static_cast<int>(vehicles.size()); i++) {
            if (vehicles[i].getId() == id) return i;
        }
        return -1;
    }

    void add(const Vehicle& vehicle) {
        if (!vehicle.isValid()) throw InvalidVehicleException("Data kendaraan tidak valid.");
        if (find(vehicle.getId()) != -1) throw DuplicateVehicleException("ID '" + vehicle.getId() + "' sudah terdaftar.");
        vehicles.push_back(vehicle);
    }

    void show(string id) const {
        int index = find(id);
        if (index == -1) throw VehicleNotFoundException("Kendaraan ID '" + id + "' tidak ditemukan.");
        vehicles[index].display();
    }

    void update(string id, const Vehicle& replacement) {
        int index = find(id);
        if (index == -1) throw VehicleNotFoundException("Kendaraan ID '" + id + "' tidak ditemukan.");
        if (replacement.getId() != id) throw InvalidVehicleException("ID kendaraan tidak boleh diubah.");
        if (!replacement.isValid()) throw InvalidVehicleException("Data pengganti tidak valid.");
        vehicles[index] = replacement;
    }

    void remove(string id) {
        int index = find(id);
        if (index == -1) throw VehicleNotFoundException("Kendaraan ID '" + id + "' tidak ditemukan.");
        vehicles.erase(vehicles.begin() + index);
    }

    void display() const {
        if (vehicles.empty()) {
            cout << "[Informasi] Fleet saat ini kosong.\n";
            return;
        }
        cout << "ID | Type | Brand | Year | Capacity | Status\n";
        cout << "-----------------------------------------------\n";
        for (const Vehicle& vehicle : vehicles) vehicle.display();
        cout << "Total: " << vehicles.size() << " kendaraan\n";
    }

    size_t size() const { return vehicles.size(); }

    void saveToCsv(const string& filename) const {
        ofstream file(filename);
        if (!file.is_open()) {
            throw FileOperationException("Gagal membuka file untuk penulisan: " + filename);
        }

        file << "id,type,brand,year,capacity,status\n";
        for (const auto& v : vehicles) {
            file << v.toCsvRow() << "\n";
        }
        file.close();
        if (file.fail()) {
            throw FileOperationException("Gagal menyimpan file: " + filename);
        }
    }

    void loadFromCsv(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            throw FileOperationException("File tidak ditemukan atau gagal dibuka: " + filename);
        }

        string header;
        const string expectedHeader = "id,type,brand,year,capacity,status";
        if (!getline(file, header)) {
            throw InvalidCsvFormatException("File CSV kosong.");
        }
        if (!header.empty() && header.back() == '\r') header.pop_back();
        if (header != expectedHeader) {
            throw InvalidCsvFormatException("Header CSV tidak sesuai spesifikasi.");
        }

        vector<Vehicle> loadedVehicles;
        set<string> loadedIds;
        string line;

        while (getline(file, line)) {
            if (line.empty()) continue;
            Vehicle v = Vehicle::fromCsvRow(line);
            if (!loadedIds.insert(v.getId()).second) {
                throw InvalidCsvFormatException("ID duplikat ditemukan di dalam file CSV: " + v.getId());
            }
            loadedVehicles.push_back(v);
        }

        if (file.bad()) {
            throw FileOperationException("Terjadi kesalahan pembacaan I/O pada file.");
        }

        // Swap hanya dilakukan apabila seluruh data CSV terbukti valid
        vehicles.swap(loadedVehicles);
    }
};

// ==========================================
// MAIN MENU INTERAKTIF
// ==========================================
int main() {
    Fleet fleet;
    int choice = -1;

    while (choice != 0) {
        cout << "\n=== FLEET MANAGEMENT ===\n";
        cout << "1. Add Vehicle\n";
        cout << "2. Show Vehicles\n";
        cout << "3. Update Vehicle\n";
        cout << "4. Remove Vehicle\n";
        cout << "5. Save Fleet to CSV\n";
        cout << "6. Load Fleet from CSV\n";
        cout << "0. Exit\n";
        cout << "Choose: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "[Error] Input tidak valid!\n";
            continue;
        }

        try {
            if (choice == 1) {
                string id, type, brand, status;
                int year, capacity;
                cout << "ID: "; cin >> id;
                cout << "Type (Car/Truck/Bus): "; cin >> type;
                cout << "Brand: "; cin >> brand;
                cout << "Year: "; cin >> year;
                cout << "Capacity: "; cin >> capacity;
                cout << "Status (ACTIVE/MAINTENANCE/INACTIVE): "; cin >> status;
                fleet.add(Vehicle(id, type, brand, year, capacity, status));
                cout << "[OK] Vehicle added successfully.\n";
            } else if (choice == 2) {
                fleet.display();
            } else if (choice == 3) {
                string id, type, brand, status;
                int year, capacity;
                cout << "ID yang ingin diupdate: "; cin >> id;
                cout << "Type Baru: "; cin >> type;
                cout << "Brand Baru: "; cin >> brand;
                cout << "Year Baru: "; cin >> year;
                cout << "Capacity Baru: "; cin >> capacity;
                cout << "Status Baru: "; cin >> status;
                fleet.update(id, Vehicle(id, type, brand, year, capacity, status));
                cout << "[OK] Vehicle updated successfully.\n";
            } else if (choice == 4) {
                string id;
                cout << "ID yang ingin dihapus: "; cin >> id;
                fleet.remove(id);
                cout << "[OK] Vehicle removed successfully.\n";
            } else if (choice == 5) {
                string filename;
                cout << "Masukkan nama file simpan [default: fleet.csv]: ";
                cin.ignore();
                getline(cin, filename);
                if (filename.empty()) filename = "fleet.csv";
                fleet.saveToCsv(filename);
                cout << "[OK] " << fleet.size() << " vehicles saved to " << filename << "\n";
            } else if (choice == 6) {
                string filename;
                cout << "Masukkan nama file muat [default: fleet.csv]: ";
                cin.ignore();
                getline(cin, filename);
                if (filename.empty()) filename = "fleet.csv";
                fleet.loadFromCsv(filename);
                cout << "[OK] " << fleet.size() << " vehicles loaded from " << filename << "\n";
            } else if (choice == 0) {
                cout << "Keluar dari program.\n";
            } else {
                cout << "[Error] Pilihan menu tidak tersedia.\n";
            }
        } catch (const FleetException& e) {
            cout << "[ERROR] " << e.what() << "\n";
        } catch (const exception& e) {
            cout << "[SYSTEM ERROR] " << e.what() << "\n";
        }
    }

    return 0;
}