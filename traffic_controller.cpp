#include "traffic_controller.hpp"

#include "initialization.hpp"

enum LoggingModes {
  PrecessConnectionInitFailed = -1,
  ProcessConnectionOperationFailed = -2,
  TruckArrived = 0,
  NotSupportedTruckInFront = 1,
  NoTruckInQueue = 2,
  TruckSendToBridge = 3,
  TrucksPassedBridge = 4,
  ControlTransferred = 5,
  ControlCannotBeTransferred = 6,
  ControlGot = 7
};

TCNS::TrafficController::TrafficController(bool location, int max_mass,
                                           int max_num_of_trucks) {
  location_ = location;
  allowed_weight_ = max_mass;
  std::string log_file = "LogTrafficController ";
  log_file += char(location_);
  if (log_.openf(log_file.c_str()) < 0) {
    Finish();
    exit(1);
  }
  try {
    message_queue_ = MessageQueue(ftok(kLogFile, int(location_) + 1));
    queue_semaphore_ = Semaphore(1, ftok(kLogFile, int(location_) + 3));
    tc_semaphore_ = Semaphore(1, ftok(kLogFile, 5));
    turn_off_semaphore_ = Semaphore(1, ftok(kLogFile, 6));

    if (turn_off_semaphore_.IsOwner()) {
      try {
        turn_off_semaphore_.Operation(0, max_num_of_trucks, false);
      } catch (int error) {
        Logging(ProcessConnectionOperationFailed, error);
        Finish();
        exit(ProcessConnectionOperationFailed);
      }
    }

    num_of_users_semaphore_ = Semaphore(2, ftok(kLogFile, 7));
    try {
      num_of_users_semaphore_.Operation(0, 1, false);
      if (location_ == factory) {
        num_of_users_semaphore_.Operation(1, 1, false);
      }
    } catch (int error) {
      Logging(ProcessConnectionOperationFailed, error);
      Finish();
      exit(ProcessConnectionOperationFailed);
    }

  } catch (int error) {
    Logging(PrecessConnectionInitFailed, error);
    Finish();
    exit(PrecessConnectionInitFailed);
  }
}

TCNS::TrafficController::~TrafficController() { Finish(); }

void TCNS::TrafficController::StartProcess() {
  if (location_ == factory) {
    try {
      tc_semaphore_.Operation(0, -1, true);
    } catch (int error) {
      Logging(ProcessConnectionOperationFailed, error);
      Finish();
      exit(ProcessConnectionOperationFailed);
    }
  }
  while (IsTurnedOn()) {
    GetTrucks();
    SendTrucksToBridgeAndWait();
    TransferControlToAnotherControllerAndWait();
  }
}

void TCNS::TrafficController::GetTrucks() {
  std::optional<Truck> truck = TruckArrival();
  if (!truck.has_value()) {
    Logging(NoTruckInQueue);
  }
  while (truck.has_value()) {
    truck_queue_.push(truck.value());
    truck = TruckArrival();
  }
}

std::optional<TCNS::Truck> TCNS::TrafficController::TruckArrival() {
  std::optional<std::pair<int, int>> raw_truck = {};
  try {
    raw_truck = message_queue_.Receive(1, false);
  } catch (int error) {
    Logging(ProcessConnectionOperationFailed, error);
    Finish();
    exit(ProcessConnectionOperationFailed);
  }
  std::optional<TCNS::Truck> result = {};
  if (raw_truck.has_value()) {
    result = {raw_truck.value().first, raw_truck.value().second};
    Logging(TruckArrived, result.value().number);
  }
  return result;
}

bool TCNS::TrafficController::SendTrucksToBridgeAndWait() {
  int curr_weight = 0;

  if (!truck_queue_.empty() && truck_queue_.front().weight > allowed_weight_) {
    Logging(NotSupportedTruckInFront, truck_queue_.front().number);
    return false;
  }

  while (!truck_queue_.empty()) {
    if (truck_queue_.front().weight + curr_weight > allowed_weight_) {
      break;
    }

    try {
      queue_semaphore_.Operation(0, 1, false);
      message_queue_.Send({0, 0}, truck_queue_.front().number + 2);
    } catch (int error) {
      Logging(ProcessConnectionOperationFailed, error);
      Finish();
      exit(ProcessConnectionOperationFailed);
    }

    Logging(TruckSendToBridge, truck_queue_.front().number);

    curr_weight += truck_queue_.front().weight;
    truck_queue_.pop();
  }

  try {
    queue_semaphore_.IsZero(0, true);
    Logging(TrucksPassedBridge);
  } catch (int error) {
    Logging(ProcessConnectionOperationFailed, error);
    Finish();
    exit(ProcessConnectionOperationFailed);
  }

  return true;
}

void TCNS::TrafficController::TransferControlToAnotherControllerAndWait() {
  try {
    if (num_of_users_semaphore_.IsZero(1, false)) {
      Logging(ControlCannotBeTransferred);
      return;
    }
    if (location_ == mine) {
      tc_semaphore_.Operation(0, 2, false);
      Logging(ControlTransferred);
      tc_semaphore_.IsZero(0, true);
      Logging(ControlGot);
    } else {
      tc_semaphore_.Operation(0, -1, false);
      Logging(ControlTransferred);
      tc_semaphore_.Operation(0, -1, true);
      Logging(ControlGot);
    }
  } catch (int error) {
    Logging(ProcessConnectionOperationFailed, error);
    Finish();
    exit(ProcessConnectionOperationFailed);
  }
}

bool TCNS::TrafficController::IsTurnedOn() {
  try {
    return !turn_off_semaphore_.IsZero(0, false);
  } catch (int error) {
    Logging(ProcessConnectionOperationFailed, error);
    Finish();
    exit(ProcessConnectionOperationFailed);
  }
}

void TCNS::TrafficController::Finish() {
  message_queue_.DeleteQueue();
  queue_semaphore_.DeleteSem();
  try {
    if (num_of_users_semaphore_.IsZero(1, false)) {
      tc_semaphore_.DeleteSem();
      while (turn_off_semaphore_.Operation(0, -1, false))
        ;
    }
    num_of_users_semaphore_.Operation(0, -1, false);
    num_of_users_semaphore_.Operation(1, -1, false);
    if (num_of_users_semaphore_.IsZero(0, false)) {
      turn_off_semaphore_.DeleteSem();
      num_of_users_semaphore_.DeleteSem();
    }
  } catch (int error) {
    tc_semaphore_.DeleteSem();
    turn_off_semaphore_.DeleteSem();
    num_of_users_semaphore_.DeleteSem();
    Logging(ProcessConnectionOperationFailed, error);
    log_.closef();
    exit(ProcessConnectionOperationFailed);
  }

  log_.closef();
}

void TCNS::TrafficController::Logging(int mode, int add_inf) {
  std::string message;
  switch (mode) {
    case PrecessConnectionInitFailed:
      message += "Не удалось создать инструменты МПВ. Код ошибки: " +
                 IntToString(add_inf) + "\n";
      break;
    case ProcessConnectionOperationFailed:
      message += "Ошибка при обращении к инструментам МПВ. Код ошибки: " +
                 IntToString(add_inf) + "\n";
      break;
    case TruckArrived:
      message +=
          "К регулировщику прибыл самосвал № " + IntToString(add_inf) + "\n";
      break;
    case NotSupportedTruckInFront:
      message +=
          "К регулировщику прибыл самосвал недопустимого веса с номером " +
          IntToString(add_inf) + "\n";
      break;
    case NoTruckInQueue:
      message = "В очереди к регулировщику не самосвалов\n";
      break;
    case TruckSendToBridge:
      message += "Самосвал № " + IntToString(add_inf) + " отправлен на мост\n";
      break;
    case TrucksPassedBridge:
      message = "Все отправленные на мост самосвалы успешно его преодолели\n";
      break;
    case ControlTransferred:
      message = "Контроль над мостом передан другому регулировщику\n";
      break;
    case ControlCannotBeTransferred:
      message =
          "Контроль не может быть передан другому регулировщика, так как его "
          "не существует\n";
      break;
    case ControlGot:
      message = "Контроль над мостом возвращён\n";
      break;
    default:
      message = "Это сообщение не должно быть выведено!\n";
  }

  if (!log_.writef(message.c_str(), message.size() + 1)) {
    std::cout << "Не удалось записать лог в файл!\n";
  }
}

std::string TCNS::TrafficController::IntToString(int number) {
  std::string result;
  if (number < 0) {
    result += '-';
    number = abs(number);
  }

  std::vector<char> reversed;
  do {
    reversed.push_back(number % 10);
    number /= 10;
  } while (number != 0);

  for (int i = reversed.size() - 1; i >= 0; --i) {
    result += reversed[i];
  }

  return result;
}