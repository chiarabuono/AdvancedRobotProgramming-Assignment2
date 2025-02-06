#ifndef OBST_PUBLISHER_HPP
#define OBST_PUBLISHER_HPP

#include "Generated/TargetsPubSubTypes.hpp"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
# include"auxfunc.h"

using namespace eprosima::fastdds::dds;

class TargetPublisher
{
private:
    Targets my_message_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    TypeSupport type_;
    MyTargets received_targets_;  // Variabile che contiene i dati ricevuti

    class PubListener : public DataWriterListener
    {
    public:
        PubListener();
        ~PubListener() override;
        void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override;
        std::atomic_int matched_;
    } listener_;

public:
    TargetPublisher();
    ~TargetPublisher();

    bool init();
    bool publish(MyTargets targets);
};

#endif // OBST_PUBLISHER_HPP
