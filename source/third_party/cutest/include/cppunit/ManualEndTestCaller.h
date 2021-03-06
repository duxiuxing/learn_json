﻿#pragma once

#include <cppunit/TestCase.h>

#include "cutest/Event.h"
#include "cutest/Runnable.h"
#include "cutest/Runner.h"

CPPUNIT_NS_BEGIN

/*
timeout_ms 单位：毫秒，默认为0，表示永不超时；
用例启动之后，如果timeout_ms之后还没有结束，则视为超时，
此时CUTEST::Runner会调用Fixture::endTest()来结束用例。
*/
template <class Fixture, unsigned int timeout_ms = 0>
class ManualEndTestCaller
  : public CUTEST_NS::Runnable
  , public TestCase
{
  typedef void ( Fixture::*TestMethod )();

public:
  ManualEndTestCaller( std::string name, TestMethod test )
    : TestCase( name )
    , m_fixture( NULL )
    , m_testMethod( test )
    , m_asyncRunMethod( ASYNC_RUN_NONE )
    , m_event( NULL )
  {}

  ~ManualEndTestCaller()
  {
    if ( m_fixture )
    {
      delete m_fixture;
    }
  }

  // 重载TestCase::runTest()
  virtual void runTest() override
  {
    m_asyncRunMethod = ASYNC_RUN_TEST;
    m_event = CUTEST_NS::Event::createInstance();
    CUTEST_NS::Runner::instance()->asyncRunOnMainThread( this, false );
    m_event->wait();
    m_event->destroy();
    m_event = NULL;
  }

  // 重载TestFixture::setUp()
  virtual void setUp() override
  {
    m_asyncRunMethod = ASYNC_RUN_SET_UP;
    m_event = CUTEST_NS::Event::createInstance();
    CUTEST_NS::Runner::instance()->asyncRunOnMainThread( this, false );
    m_event->wait();
    m_event->destroy();
    m_event = NULL;
  }

  // 重载TestFixture::tearDown()
  virtual void tearDown() override
  {
    m_asyncRunMethod = ASYNC_RUN_TEAR_DOWN;
    m_event = CUTEST_NS::Event::createInstance();
    CUTEST_NS::Runner::instance()->asyncRunOnMainThread( this, false );
    m_event->wait();
    m_event->destroy();
    m_event = NULL;
  }

  // 实现Runnable::run()
  virtual void run()
  {
    switch ( m_asyncRunMethod )
    {
    case ASYNC_RUN_SET_UP:
      {
        if ( !m_fixture )
        {
          m_fixture = new Fixture;
        }
        m_fixture->setUp();
        m_event->post();
      }
      break;
    case ASYNC_RUN_TEAR_DOWN:
      {
        m_fixture->tearDown();
        m_event->post();
      }
      break;
    case ASYNC_RUN_TEST:
      {
        m_fixture->setEvent( m_event );
        CUTEST_NS::Runner::instance()->registerManualEndTest( m_fixture, timeout_ms );
        ( m_fixture->*m_testMethod )();
      }
      break;
    default:
      break;
    }
  }

private:
  ManualEndTestCaller( const ManualEndTestCaller &other );
  ManualEndTestCaller &operator =( const ManualEndTestCaller &other );

  Fixture *m_fixture;
  TestMethod m_testMethod;

  // 用于标识在run()方法中要调用的方法
  enum AsyncRunMethod
  {
    ASYNC_RUN_NONE = 0,     // 初始值，无意义
    ASYNC_RUN_SET_UP,       // m_fixture->setUp();
    ASYNC_RUN_TEAR_DOWN,    // m_fixture->tearDown();
    ASYNC_RUN_TEST,         // (m_fixture->*m_test)();
  };
  AsyncRunMethod m_asyncRunMethod;

  CUTEST_NS::Event *m_event;
};

CPPUNIT_NS_END
