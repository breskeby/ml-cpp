/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License;
 * you may not use this file except in compliance with the Elastic License.
 */

#ifndef INCLUDED_ml_model_CBucketGatherer_h
#define INCLUDED_ml_model_CBucketGatherer_h

#include <core/CLogger.h>
#include <core/CMemory.h>
#include <core/CCompressedDictionary.h>
#include <core/CHashing.h>
#include <core/CoreTypes.h>
#include <core/CStoredStringPtr.h>

#include <model/CBucketQueue.h>
#include <model/CEventData.h>
#include <model/CModelParams.h>
#include <model/FunctionTypes.h>
#include <model/ImportExport.h>
#include <model/ModelTypes.h>

#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/functional/hash.hpp>

#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <stdint.h>


namespace ml
{
namespace core
{
class CStatePersistInserter;
class CStateRestoreTraverser;
}
namespace model
{
class CDataGatherer;
class CEventData;
class CSearchKey;
class CResourceMonitor;

//! \brief Time series data gathering interface.
//!
//! DESCRIPTION:\n
//! This defines the interface to gather time-specific data for one or more
//! time series.
//!
//! This is subclassed by Metric and EventRate implementations.
//!
//! IMPLEMENTATION:\n
//! This functionality has been separated from the CDataGatherer in order
//! to allow the CDataGatherer to support multiple overlapping buckets and
//! buckets with different time spans.
class MODEL_EXPORT CBucketGatherer
{
    public:
        typedef std::vector<double> TDoubleVec;
        typedef core::CSmallVector<double, 1> TDouble1Vec;
        typedef std::vector<std::size_t> TSizeVec;
        typedef std::vector<std::string> TStrVec;
        typedef TStrVec::const_iterator TStrVecCItr;
        typedef std::vector<const std::string*> TStrCPtrVec;
        typedef std::pair<std::size_t, uint64_t> TSizeUInt64Pr;
        typedef std::vector<TSizeUInt64Pr> TSizeUInt64PrVec;
        typedef model_t::TFeatureVec TFeatureVec;
        typedef boost::optional<double> TOptionalDouble;
        typedef std::pair<std::size_t, std::size_t> TSizeSizePr;
        typedef std::pair<TSizeSizePr, uint64_t> TSizeSizePrUInt64Pr;
        typedef std::vector<TSizeSizePrUInt64Pr> TSizeSizePrUInt64PrVec;
        typedef core::CCompressedDictionary<2> TDictionary;
        typedef TDictionary::CWordUMap<std::size_t>::Type TWordSizeUMap;
        typedef TWordSizeUMap::iterator TWordSizeUMapItr;
        typedef TWordSizeUMap::const_iterator TWordSizeUMapCItr;
        typedef boost::unordered_map<TSizeSizePr, uint64_t> TSizeSizePrUInt64UMap;
        typedef TSizeSizePrUInt64UMap::iterator TSizeSizePrUInt64UMapItr;
        typedef TSizeSizePrUInt64UMap::const_iterator TSizeSizePrUInt64UMapCItr;
        typedef CBucketQueue<TSizeSizePrUInt64UMap> TSizeSizePrUInt64UMapQueue;
        typedef std::map<core_t::TTime, TSizeSizePrUInt64UMap> TTimeSizeSizePrUInt64UMapMap;
        typedef TSizeSizePrUInt64UMapQueue::iterator TSizeSizePrUInt64UMapQueueItr;
        typedef TSizeSizePrUInt64UMapQueue::const_iterator TSizeSizePrUInt64UMapQueueCItr;
        typedef TSizeSizePrUInt64UMapQueue::const_reverse_iterator TSizeSizePrUInt64UMapQueueCRItr;
        typedef boost::unordered_set<TSizeSizePr> TSizeSizePrUSet;
        typedef TSizeSizePrUSet::const_iterator TSizeSizePrUSetCItr;
        typedef CBucketQueue<TSizeSizePrUSet> TSizeSizePrUSetQueue;
        typedef std::map<core_t::TTime, TSizeSizePrUSet> TTimeSizeSizePrUSetMap;
        typedef TSizeSizePrUSetQueue::const_iterator TSizeSizePrUSetQueueCItr;
        typedef std::vector<core::CStoredStringPtr> TStoredStringPtrVec;
        typedef std::pair<TSizeSizePr, core::CStoredStringPtr> TSizeSizePrStoredStringPtrPr;

        //! \brief Hashes a ((size_t, size_t), string*) pair.
        struct MODEL_EXPORT SSizeSizePrStoredStringPtrPrHash
        {
            std::size_t operator()(const TSizeSizePrStoredStringPtrPr &key) const
            {
                uint64_t seed = core::CHashing::hashCombine(static_cast<uint64_t>(key.first.first),
                                                            static_cast<uint64_t>(key.first.second));
                return core::CHashing::hashCombine(seed, s_Hasher(*key.second));
            }
            core::CHashing::CMurmurHash2String s_Hasher;
        };

        //! \brief Checks two ((size_t, size_t), string*) pairs for equality.
        struct MODEL_EXPORT SSizeSizePrStoredStringPtrPrEqual
        {
            bool operator()(const TSizeSizePrStoredStringPtrPr &lhs,
                            const TSizeSizePrStoredStringPtrPr &rhs) const
            {
                return lhs.first == rhs.first && *lhs.second == *rhs.second;
            }
        };

        typedef boost::unordered_map<TSizeSizePrStoredStringPtrPr,
                                     uint64_t,
                                     SSizeSizePrStoredStringPtrPrHash,
                                     SSizeSizePrStoredStringPtrPrEqual> TSizeSizePrStoredStringPtrPrUInt64UMap;
        typedef TSizeSizePrStoredStringPtrPrUInt64UMap::const_iterator TSizeSizePrStoredStringPtrPrUInt64UMapCItr;
        typedef TSizeSizePrStoredStringPtrPrUInt64UMap::iterator TSizeSizePrStoredStringPtrPrUInt64UMapItr;
        typedef std::vector<TSizeSizePrStoredStringPtrPrUInt64UMap> TSizeSizePrStoredStringPtrPrUInt64UMapVec;
        typedef CBucketQueue<TSizeSizePrStoredStringPtrPrUInt64UMapVec> TSizeSizePrStoredStringPtrPrUInt64UMapVecQueue;
        typedef TSizeSizePrStoredStringPtrPrUInt64UMapVec::const_iterator TSizeSizePrStoredStringPtrPrUInt64UMapVecCItr;
        typedef std::map<core_t::TTime, TSizeSizePrStoredStringPtrPrUInt64UMapVec> TTimeSizeSizePrStoredStringPtrPrUInt64UMapVecMap;
        typedef boost::reference_wrapper<const CSearchKey> TSearchKeyCRef;
        typedef std::pair<model_t::EFeature, boost::any> TFeatureAnyPr;
        typedef std::vector<TFeatureAnyPr> TFeatureAnyPrVec;
        typedef std::vector<model_t::EMetricCategory> TMetricCategoryVec;
        typedef std::vector<core_t::TTime> TTimeVec;
        typedef TTimeVec::const_iterator TTimeVecCItr;

    public:
        static const std::string EVENTRATE_BUCKET_GATHERER_TAG;
        static const std::string METRIC_BUCKET_GATHERER_TAG;

    public:
        //! \name Life-cycle
        //@{
        //! Create a new data series gatherer.
        //!
        //! \param[in] dataGatherer The owning data gatherer.
        //! \param[in] startTime The start of the time interval
        //! for which to gather data.
        CBucketGatherer(CDataGatherer &dataGatherer,
                        core_t::TTime startTime);

        //! Create a copy that will result in the same persisted state as the
        //! original.  This is effectively a copy constructor that creates a
        //! copy that's only valid for a single purpose.  The boolean flag is
        //! redundant except to create a signature that will not be mistaken for
        //! a general purpose copy constructor.
        CBucketGatherer(bool isForPersistence,
                        const CBucketGatherer &other);

        virtual ~CBucketGatherer(void) = default;
        //@}

        //! \name Persistence
        //@{
        //! Persist state by passing information to the supplied inserter
        virtual void baseAcceptPersistInserter(core::CStatePersistInserter &inserter) const;

        //! Restore the state
        virtual bool baseAcceptRestoreTraverser(core::CStateRestoreTraverser &traverser);

        //! Create a clone of this data gatherer that will result in the same
        //! persisted state.  The clone may be incomplete in ways that do not
        //! affect the persisted representation, and must not be used for any
        //! other purpose.
        //! \warning The caller owns the object returned.
        virtual CBucketGatherer *cloneForPersistence(void) const = 0;

        //! The persistence tag name of the subclass.
        virtual const std::string& persistenceTag(void) const = 0;
        //@}

        //! \name Fields
        //@{
        //! This is the common field in all searches "along" which the
        //! probabilities are aggregated, i.e. the "by" field name for
        //! individual models and the "over" field name for population
        //! models.
        virtual const std::string &personFieldName(void) const = 0;

        //! Get the attribute field name if one exists.
        virtual const std::string &attributeFieldName(void) const = 0;

        //! Get the name of the field containing the metric value.
        virtual const std::string &valueFieldName(void) const = 0;

        //! Get an iterator at the beginning the influencing field names.
        virtual TStrVecCItr beginInfluencers(void) const = 0;

        //! Get an iterator at the end of the influencing field names.
        virtual TStrVecCItr endInfluencers(void) const = 0;

        //! Get the fields for which to gather data.
        //!
        //! This defines the fields to extract from a record. These include
        //! the fields which define the categories whose counts are being
        //! analyzed, the fields containing metric series names and values
        //! and the fields defining a population.
        virtual const TStrVec &fieldsOfInterest(void) const = 0;
        //@}

        //! Get a description of the component searches.
        virtual std::string description(void) const = 0;

        //! \name Update
        //@{
        //! Process the specified fields.
        //!
        //! This adds people and attributes as necessary and fills out the
        //! event data from \p fieldValues.
        virtual bool processFields(const TStrCPtrVec &fieldValues,
                                   CEventData &result,
                                   CResourceMonitor &resourceMonitor) = 0;

        //! Record the arrival of \p data at \p time.
        bool addEventData(CEventData &data);

        //! Roll time forwards to \p time.
        void timeNow(core_t::TTime time);

        //! Roll time to the end of the bucket that is latency after the sampled bucket.
        void sampleNow(core_t::TTime sampleBucketStart);

        //! Roll time to the end of the bucket that is latency after the sampled bucket
        //! without performing any updates that impact the model.
        void skipSampleNow(core_t::TTime sampleBucketStart);
        //@}

        //! \name People
        //@{
        //! Get the non-zero counts by person for the bucketing interval
        //! containing \p time.
        //!
        //! \param[in] time The time of interest.
        //! \param[out] result Filled in with the non-zero counts by person.
        //! The first element is the person identifier and the second their
        //! count in the bucketing interval. The result is sorted by person.
        //! \note We expect the non-zero counts to be sparse on the space
        //! of people so use a sparse encoding:
        //! <pre class="fragment">
        //!   \f$ pid \leftarrow c\f$
        //! </pre>
        //! where,\n
        //!   \f$pid\f$ is the person identifier,\n
        //!   \f$c\f$ is the count for the person.
        void personNonZeroCounts(core_t::TTime time,
                                 TSizeUInt64PrVec &result) const;

        //! Stop gathering data on the people identified by \p peopleToRemove.
        virtual void recyclePeople(const TSizeVec &peopleToRemove) = 0;

        //! Remove all traces of people whose identifiers are greater than
        //! or equal to \p lowestPersonToRemove.
        virtual void removePeople(std::size_t lowestPersonToRemove) = 0;
        //@}

        //! \name Attribute
        //@{
        //! Stop gathering data on the attributes identified by \p attributesToRemove.
        virtual void recycleAttributes(const TSizeVec &attributesToRemove) = 0;

        //! Remove all traces of attributes whose identifiers are greater than
        //! or equal to \p lowestAttributeToRemove.
        virtual void removeAttributes(std::size_t lowestAttributeToRemove) = 0;
        //@}

        //! \name Time
        //@{
        //! Get the start of the current bucketing time interval.
        core_t::TTime currentBucketStartTime(void) const;

        //! Set the start of the current bucketing time interval.
        void currentBucketStartTime(core_t::TTime time);

        //! The earliest time for which data can still arrive.
        core_t::TTime earliestBucketStartTime(void) const;

        //! Get the length of the bucketing time interval.
        core_t::TTime bucketLength(void) const;

        //! Check if data is available at \p time.
        bool dataAvailable(core_t::TTime time) const;

        //! For each bucket in the interval [\p startTime, \p endTime],
        //! validate that it can be sampled and increment \p startTime
        //! to the first valid bucket or \p endTime if no valid buckets
        //! exist.
        //!
        //! \param[in,out] startTime The start of the interval to sample.
        //! \param[in] endTime The end of the interval to sample.
        bool validateSampleTimes(core_t::TTime &startTime,
                                 core_t::TTime endTime) const;

        //! Print the current bucket.
        std::string printCurrentBucket(void) const;
        //@}

        //! \name Counts
        //@{
        //! Get the non-zero (person, attribute) pair counts in the
        //! bucketing interval corresponding to the given time.
        const TSizeSizePrUInt64UMap &bucketCounts(core_t::TTime time) const;

        //! Get the non-zero (person, attribute) pair counts for each
        //! value of influencing field.
        const TSizeSizePrStoredStringPtrPrUInt64UMapVec &influencerCounts(core_t::TTime time) const;
        //@}

        //! Get the checksum of this gatherer.
        virtual uint64_t checksum(void) const = 0;

        //! Debug the memory used by this component.
        virtual void debugMemoryUsage(core::CMemoryUsage::TMemoryUsagePtr mem) const = 0;

        //! Get the memory used by this component.
        virtual std::size_t memoryUsage(void) const = 0;

        //! Get the static size of this object.
        virtual std::size_t staticSize(void) const = 0;

        //! Clear this data gatherer.
        virtual void clear(void) = 0;

        //! Reset bucket and return true if bucket was successfully
        //! reset or false otherwise.
        virtual bool resetBucket(core_t::TTime bucketStart) = 0;

        //! Release memory that is no longer needed
        virtual void releaseMemory(core_t::TTime samplingCutoffTime) = 0;

        //! Remove the values in queue for the people or attributes
        //! in \p toRemove.
        //!
        //! \tparam T This must be an associative array from person
        //! id and/or attribute id to some corresponding value.
        template<typename F, typename T>
        static void remove(const TSizeVec &toRemove,
                           const F &extractId,
                           CBucketQueue<T> &queue)
        {
            typedef typename CBucketQueue<T>::iterator TQueueItr;

            for (TQueueItr bucketItr = queue.begin(); bucketItr != queue.end(); ++bucketItr)
            {
                T &bucket = *bucketItr;
                for (typename T::iterator i = bucket.begin(); i != bucket.end(); /**/)
                 {
                     if (std::binary_search(toRemove.begin(), toRemove.end(), extractId(*i)))
                     {
                         i = bucket.erase(i);
                     }
                     else
                     {
                         ++i;
                     }
                 }
            }
        }

        //! Remove the values in queue for the people or attributes
        //! in \p toRemove.
        //!
        //! \tparam T This must be a vector of associative array from person
        //! id and/or attribute id to some corresponding value.
        template<typename F, typename T>
        static void remove(const TSizeVec &toRemove,
                           const F &extractId,
                           CBucketQueue<std::vector<T> > &queue)
        {
            typedef typename CBucketQueue<std::vector<T> >::iterator TQueueItr;

            for (TQueueItr bucketItr = queue.begin(); bucketItr != queue.end(); ++bucketItr)
            {
                for (std::size_t i = 0u; i < bucketItr->size(); ++i)
                {
                    T &bucket = (*bucketItr)[i];
                    for (typename T::iterator j = bucket.begin(); j != bucket.end(); /**/)
                    {
                        if (std::binary_search(toRemove.begin(), toRemove.end(), extractId(j->first)))
                        {
                            j = bucket.erase(j);
                        }
                        else
                        {
                            ++j;
                        }
                    }
                }
            }
        }

        //! Get the raw data for all features for the bucketing time interval
        //! containing \p time.
        //!
        //! \param[in] time The time of interest.
        //! \param[out] result Filled in with the feature data at \p time.
        virtual void featureData(core_t::TTime time, core_t::TTime bucketLength,
                                 TFeatureAnyPrVec &result) const = 0;

        //! Get a reference to the owning data gatherer.
        const CDataGatherer &dataGatherer(void) const;

        //! Has this pid/cid pair had only explicit null records?
        bool hasExplicitNullsOnly(core_t::TTime time, std::size_t pid, std::size_t cid) const;

        //! Create samples if possible for the bucket pointed out by \p time.
        virtual void sample(core_t::TTime time);

    private:
        //! Resize the necessary data structures so they can hold values
        //! for the person and/or attribute identified by \p pid and \p cid,
        //! respectively.
        //!
        //! \param[in] pid The identifier of the person to accommodate.
        //! \param[in] cid The identifier of the attribute to accommodate.
        virtual void resize(std::size_t pid, std::size_t cid) = 0;

        //! Record the arrival of \p values for attribute identified
        //! by \p cid and person identified by \p pid.
        //!
        //! \param[in] pid The identifier of the person who generated
        //! the value.
        //! \param[in] cid The identifier of the value's attribute.
        //! \param[in] time The time of the \p values.
        //! \param[in] values The metric statistic value(s).
        //! \param[in] count The number of measurements in the metric
        //! statistic.
        //! \param[in] stringValue The value for the function string argument
        //! if there is one or null.
        //! \param[in] influences The influencing field values which label
        //! the value.
        virtual void addValue(std::size_t pid,
                              std::size_t cid,
                              core_t::TTime time,
                              const CEventData::TDouble1VecArray &values,
                              std::size_t count,
                              const CEventData::TOptionalStr &stringValue,
                              const TStoredStringPtrVec &influences) = 0;

        //! Handle the start of a new bucketing interval.
        virtual void startNewBucket(core_t::TTime time, bool skipUpdates) = 0;

        //! Roll time forwards to \p time and update depending on \p skipUpdates
        void hiddenTimeNow(core_t::TTime time, bool skipUpdates);

    protected:
        //! Reference to the owning data gatherer
        CDataGatherer &m_DataGatherer;

    private:
        //! The earliest time of any record that has arrived.
        core_t::TTime m_EarliestTime;

        //! The start of the current bucketing interval.
        core_t::TTime m_BucketStart;

        //! The non-zero (person, attribute) pair counts in the current
        //! bucketing interval.
        TSizeSizePrUInt64UMapQueue m_PersonAttributeCounts;

        //! The counts for longer bucketing intervals.
        // TODO This is not queued so can't handle out of order data.
        TTimeSizeSizePrUInt64UMapMap m_MultiBucketPersonAttributeCounts;

        //! A set per bucket that contains a (pid,cid) pair if at least
        //! one explicit null record has been seen.
        TSizeSizePrUSetQueue m_PersonAttributeExplicitNulls;

        //! The explicit nulls for longer bucketing intervals.
        // TODO This is not queued so can't handle out of order data.
        TTimeSizeSizePrUSetMap m_MultiBucketPersonAttributeExplicitNulls;

        //! The influencing field value counts per person and/or attribute.
        TSizeSizePrStoredStringPtrPrUInt64UMapVecQueue m_InfluencerCounts;

        //! The influencing field value counts for longer bucketing intervals.
        // TODO This is not queued so can't handle out of order data.
        TTimeSizeSizePrStoredStringPtrPrUInt64UMapVecMap m_MultiBucketInfluencerCounts;
};

}
}

#endif // INCLUDED_ml_model_CBucketGatherer_h