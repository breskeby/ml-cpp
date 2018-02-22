/*
 * Copyright Elasticsearch B.V. and/or licensed to Elasticsearch B.V. under one
 * or more contributor license agreements. Licensed under the Elastic License;
 * you may not use this file except in compliance with the Elastic License.
 */

#include "CCompressedDictionaryTest.h"

#include <core/CCompressedDictionary.h>
#include <core/CLogger.h>

#include <test/CRandomNumbers.h>

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

using namespace ml;
using namespace core;
using namespace test;

void CCompressedDictionaryTest::testAll(void)
{
    typedef std::vector<std::string> TStrVec;
    typedef CCompressedDictionary<2> TDictionary;
    typedef TDictionary::TWordUSet TWordUSet;

    // Don't set this too high as it slows down every build - it can be
    // temporarily set high in uncommitted code for a thorough soak test
    // following changes to the class being tested
    const std::size_t numberTests = 10u;
    const std::size_t wordLength = 16u;
    const std::size_t numberWords = 500000u;

    CRandomNumbers rng;
    TStrVec words;

    std::string word2("word2");
    std::string word3("word3");

    for (std::size_t i = 0u; i < numberTests; ++i)
    {
        LOG_DEBUG("Collision test = " << i);

        rng.generateWords(wordLength, numberWords, words);

        TDictionary dictionary;

        TWordUSet uniqueWords;
        for (std::size_t j = 0u; j < words.size(); ++j)
        {
            CPPUNIT_ASSERT(uniqueWords.insert(dictionary.word(words[j])).second);
            CPPUNIT_ASSERT(uniqueWords.insert(dictionary.word(words[j], word2)).second);
            CPPUNIT_ASSERT(uniqueWords.insert(dictionary.word(words[j], word2, word3)).second);
        }
    }
}

void CCompressedDictionaryTest::testPersist(void)
{
    typedef CCompressedDictionary<1> TDictionary1;
    typedef CCompressedDictionary<2> TDictionary2;
    typedef CCompressedDictionary<3> TDictionary3;
    typedef CCompressedDictionary<4> TDictionary4;

    {
        TDictionary1 dictionary;
        TDictionary1::CWord word = dictionary.word("hello");
        const std::string representation = word.toDelimited();
        word = dictionary.word("blank");
        CPPUNIT_ASSERT(dictionary.word("special") != word);
        CPPUNIT_ASSERT(word.fromDelimited(representation));
        CPPUNIT_ASSERT(dictionary.word("hello") == word);
    }
    {
        TDictionary2 dictionary;
        TDictionary2::CWord word = dictionary.word("world");
        const std::string representation = word.toDelimited();
        word = dictionary.word("blank");
        CPPUNIT_ASSERT(dictionary.word("special") != word);
        CPPUNIT_ASSERT(word.fromDelimited(representation));
        CPPUNIT_ASSERT(dictionary.word("world") == word);
    }
    {
        TDictionary3 dictionary;
        TDictionary3::CWord word = dictionary.word("special");
        const std::string representation = word.toDelimited();
        word = dictionary.word("blank");
        CPPUNIT_ASSERT(dictionary.word("special") != word);
        CPPUNIT_ASSERT(word.fromDelimited(representation));
        CPPUNIT_ASSERT(dictionary.word("special") == word);
    }
    {
        TDictionary4 dictionary;
        TDictionary4::CWord word = dictionary.word("TEST");
        const std::string representation = word.toDelimited();
        word = dictionary.word("blank");
        CPPUNIT_ASSERT(dictionary.word("special") != word);
        CPPUNIT_ASSERT(word.fromDelimited(representation));
        CPPUNIT_ASSERT(dictionary.word("TEST") == word);
    }
}

CppUnit::Test *CCompressedDictionaryTest::suite(void)
{
    CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite("CCompressedDictionaryTest");

    suiteOfTests->addTest( new CppUnit::TestCaller<CCompressedDictionaryTest>(
                                   "CCompressedDictionaryTest::testAll",
                                   &CCompressedDictionaryTest::testAll) );
    suiteOfTests->addTest( new CppUnit::TestCaller<CCompressedDictionaryTest>(
                                   "CCompressedDictionaryTest::testPersist",
                                   &CCompressedDictionaryTest::testPersist) );
    return suiteOfTests;
}