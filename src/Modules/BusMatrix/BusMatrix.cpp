#include "BusMatrix.h"

#include <algorithm>

Sim::BusMatrix::BusMatrix(sc_module_name nm)
    : sc_module(nm) {
    SC_THREAD(main_action);
    sensitive << clk_i.neg();
}

void Sim::BusMatrix::main_action() {
    while (true) {
        wait();

        if (this->requestsQueue_.empty())
            continue;

        // Getting the next reqest. If we were locked by a master then only allow reqests from that master
        Request* req;
        if (this->isLocked) {
            auto res = std::find(this->requestsQueue_.begin(), this->requestsQueue_.end(), this->lockedMasterId);
            if (res == this->requestsQueue_.end())
                continue;  // No request from blocked master
            req = this->requests_[this->lockedMasterId];
            this->requestsQueue_.erase(res);
        } else {
            req = this->requests_[this->requestsQueue_.front()];
            this->requestsQueue_.pop_front();
        }

        // Getting the slave
        IBusMatrixSlave* slave = nullptr;
        for (int i = 0; i < slave_port.size(); i++) {
            if (req->addr >= slave_port[i]->beginAddress() && req->addr <= slave_port[i]->endAddress()) {
                slave = slave_port[i];
                break;
            }
        }
        sc_assert(slave != nullptr);

        // Processing
        this->isLocked = req->lock;
        this->lockedMasterId = req->masterId;
        if (req->isRead) {
            slave->read(req->addr, req->data, req->length);
        } else {
            slave->write(req->addr, req->data, req->length, req->mask);
        }
        for (int i = 0; i < req->length - 1; i++) {
            wait();  // move time
        }
        req->doneEvent.notify();
    }
}

void Sim::BusMatrix::read(size_t masterId, uint32_t addr, uint64_t* data, bool lock) {
    this->burst_read(masterId, addr, data, 1, lock);
}
void Sim::BusMatrix::write(size_t masterId, uint32_t addr, const uint64_t data, uint8_t mask, bool lock) {
    this->burst_write(masterId, addr, &data, 1, mask, lock);
}

void Sim::BusMatrix::burst_read(size_t masterId, uint32_t addr, uint64_t* data, size_t length, bool lock) {
    auto req = this->getRequestElement(masterId);

    req->masterId = masterId;
    req->addr = addr;
    req->data = const_cast<uint64_t*>(data);
    req->length = length;
    req->lock = lock;
    req->isRead = true;
    this->requestsQueue_.push_back(masterId);

    wait(req->doneEvent);
    wait(this->clk_i->posedge_event());
    return;
}

void Sim::BusMatrix::burst_write(size_t masterId, uint32_t addr, const uint64_t* data, size_t length, uint8_t mask, bool lock) {
    auto req = this->getRequestElement(masterId);

    req->masterId = masterId;
    req->addr = addr;
    req->data = const_cast<uint64_t*>(data);
    req->length = length;
    req->mask = mask;
    req->lock = lock;
    req->isRead = false;
    this->requestsQueue_.push_back(masterId);

    wait(req->doneEvent);
    wait(this->clk_i->posedge_event());
    return;
}

Sim::BusMatrix::Request* Sim::BusMatrix::getRequestElement(size_t masterId) {
    if (!this->requests_.contains(masterId)) {
        this->requests_.insert({masterId, new Request()});
    }
    return this->requests_[masterId];
}
