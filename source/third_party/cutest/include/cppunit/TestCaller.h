﻿#ifndef CPPUNIT_TESTCALLER_H    // -*- C++ -*-
#define CPPUNIT_TESTCALLER_H

#include <cppunit/Exception.h>
#include <cppunit/Protector.h>
#include <cppunit/TestCase.h>

#include "cutest/Event.h"
#include "cutest/Runnable.h"
#include "cutest/Runner.h"

#if CPPUNIT_USE_TYPEINFO_NAME
#  include <cppunit/extensions/TypeInfoHelper.h>
#endif


CPPUNIT_NS_BEGIN

#if 0
/*! \brief Marker class indicating that no exception is expected by TestCaller.
 * This class is an implementation detail. You should never use this class directly.
 */
class CPPUNIT_API NoExceptionExpected
{
private:
  //! Prevent class instantiation.
  NoExceptionExpected();
};


/*! \brief (Implementation) Traits used by TestCaller to expect an exception.
 *
 * This class is an implementation detail. You should never use this class directly.
 */
template<class ExceptionType>
struct ExpectedExceptionTraits
{
  static void expectedException()
  {
#if CPPUNIT_USE_TYPEINFO_NAME
    throw Exception( Message(
                         "expected exception not thrown",
                         "Expected exception type: " + 
                           TypeInfoHelper::getClassName( typeid( ExceptionType ) ) ) );
#else
    throw Exception( "expected exception not thrown" );
#endif
  }
};


/*! \brief (Implementation) Traits specialization used by TestCaller to 
 * expect no exception.
 *
 * This class is an implementation detail. You should never use this class directly.
 */
template<>
struct ExpectedExceptionTraits<NoExceptionExpected>
{
  static void expectedException()
  {
  }
};


#endif

//*** FIXME: rework this when class Fixture is implemented. ***//


/*! \brief Generate a test case from a fixture method.
 * \ingroup WritingTestFixture
 *
 * A test caller provides access to a test case method 
 * on a test fixture class.  Test callers are useful when 
 * you want to run an individual test or add it to a 
 * suite.
 * Test Callers invoke only one Test (i.e. test method) on one 
 * Fixture of a TestFixture.
 * 
 * Here is an example:
 * \code
 * class MathTest : public CppUnit::TestFixture {
 *         ...
 *     public:
 *         void         setUp();
 *         void         tearDown();
 *
 *         void         testAdd();
 *         void         testSubtract();
 * };
 *
 * CppUnit::Test *MathTest::suite() {
 *     CppUnit::TestSuite *suite = new CppUnit::TestSuite;
 *
 *     suite->addTest( new CppUnit::TestCaller<MathTest>( "testAdd", testAdd ) );
 *     return suite;
 * }
 * \endcode
 *
 * You can use a TestCaller to bind any test method on a TestFixture
 * class, as long as it accepts void and returns void.
 * 
 * \see TestCase
 */
#if 0
template <class Fixture>
class TestCaller : public TestCase
{ 
  typedef void (Fixture::*TestMethod)();
    
public:
  /*!
   * Constructor for TestCaller. This constructor builds a new Fixture
   * instance owned by the TestCaller.
   * \param name name of this TestCaller
   * \param test the method this TestCaller calls in runTest()
   */
  TestCaller( std::string name, TestMethod test ) :
	    TestCase( name ), 
	    m_ownFixture( true ),
	    m_fixture( new Fixture() ),
	    m_test( test )
  {
  }

  /*!
   * Constructor for TestCaller. 
   * This constructor does not create a new Fixture instance but accepts
   * an existing one as parameter. The TestCaller will not own the
   * Fixture object.
   * \param name name of this TestCaller
   * \param test the method this TestCaller calls in runTest()
   * \param fixture the Fixture to invoke the test method on.
   */
  TestCaller(std::string name, TestMethod test, Fixture& fixture) :
	    TestCase( name ), 
	    m_ownFixture( false ),
	    m_fixture( &fixture ),
	    m_test( test )
  {
  }
    
  /*!
   * Constructor for TestCaller. 
   * This constructor does not create a new Fixture instance but accepts
   * an existing one as parameter. The TestCaller will own the
   * Fixture object and delete it in its destructor.
   * \param name name of this TestCaller
   * \param test the method this TestCaller calls in runTest()
   * \param fixture the Fixture to invoke the test method on.
   */
  TestCaller(std::string name, TestMethod test, Fixture* fixture) :
	    TestCase( name ), 
	    m_ownFixture( true ),
	    m_fixture( fixture ),
	    m_test( test )
  {
  }
    
  ~TestCaller() 
  {
    if (m_ownFixture)
      delete m_fixture;
  }

  void runTest()
  { 
//	  try {
	    (m_fixture->*m_test)();
//	  }
//	  catch ( ExpectedException & ) {
//	    return;
//	  }

//  	ExpectedExceptionTraits<ExpectedException>::expectedException();
  }  

  void setUp()
  { 
  	m_fixture->setUp (); 
  }

  void tearDown()
  { 
	  m_fixture->tearDown (); 
  }

  std::string toString() const
  { 
  	return "TestCaller " + getName(); 
  }

private: 
  TestCaller( const TestCaller &other ); 
  TestCaller &operator =( const TestCaller &other );

private:
  bool m_ownFixture;
  Fixture *m_fixture;
  TestMethod m_test;
};
#endif

template <class Fixture>
class TestCaller
  : public CUTEST_NS::Runnable
  , public TestCase
{
  typedef void ( Fixture::*TestMethod )();
  class MethodFunctor : public Functor
  {
  public:
    typedef void ( TestCaller::*Method )();

    MethodFunctor( TestCaller *target, Method method )
      : m_target( target )
      , m_method( method )
    {}

    bool operator()() const
    {
      ( m_target->*m_method )();
      return true;
    }

  private:
    TestCaller *m_target;
    Method m_method;
  };

public:
  /*!
   * Constructor for TestCaller. This constructor builds a new Fixture
   * instance owned by the TestCaller.
   * \param name name of this TestCaller
   * \param test the method this TestCaller calls in runTest()
   */
  TestCaller( std::string name, TestMethod test ) :
    TestCase( name ),
    m_ownFixture( true ),
    m_fixture( NULL ),
    m_test( test ),
    m_asyncRunMethod( ASYNC_RUN_NONE ),
    m_event( NULL )
  {}

  virtual void runTest() override
  {
    m_asyncRunMethod = ASYNC_RUN_TEST;
    if ( CUTEST_NS::Runner::currentThreadId() == CUTEST_NS::Runner::mainThreadId() )
    {
      run();
    }
    else
    {
      m_event = CUTEST_NS::Event::createInstance();
      CUTEST_NS::Runner::instance()->asyncRunOnMainThread( this, false );
      m_event->wait();
      m_event->destroy();
      m_event = NULL;
    }
  }

  virtual void setUp() override
  {
    m_asyncRunMethod = ASYNC_RUN_SET_UP;
    if ( CUTEST_NS::Runner::currentThreadId() == CUTEST_NS::Runner::mainThreadId() )
    {
      run();
    }
    else
    {
      m_event = CUTEST_NS::Event::createInstance();
      CUTEST_NS::Runner::instance()->asyncRunOnMainThread( this, false );
      m_event->wait();
      m_event->destroy();
      m_event = NULL;
    }
  }

  void setUpOnMainThread()
  {
    if ( m_ownFixture )
    {
      m_fixture = new Fixture;
    }
    m_fixture->setUp();
  }

  virtual void tearDown() override
  {
    m_asyncRunMethod = ASYNC_RUN_TEAR_DOWN;
    if ( CUTEST_NS::Runner::currentThreadId() == CUTEST_NS::Runner::mainThreadId() )
    {
      run();
    }
    else
    {
      m_event = CUTEST_NS::Event::createInstance();
      CUTEST_NS::Runner::instance()->asyncRunOnMainThread( this, false );
      m_event->wait();
      m_event->destroy();
      m_event = NULL;
    }
  }

  void tearDownOnMainThread()
  {
    m_fixture->tearDown();
    if ( m_ownFixture )
    {
      delete m_fixture;
      m_fixture = NULL;
    }
  }

  void runTestOnMainThread()
  {
    ( m_fixture->*m_test )();
  }

  std::string toString() const
  {
    return "TestCaller " + getName();
  }

  // 实现Runnable::run()
  virtual void run()
  {
    switch ( m_asyncRunMethod )
    {
    case ASYNC_RUN_SET_UP:
      {
        if ( m_result )
          m_result->protect( MethodFunctor( this, &TestCaller::setUpOnMainThread ),
                             this, "setUp() failed" );
        else
        {
          setUpOnMainThread();
        }
      }
      break;
    case ASYNC_RUN_TEAR_DOWN:
      {
        if ( m_result )
          m_result->protect( MethodFunctor( this, &TestCaller::tearDownOnMainThread ),
                             this, "tearDown() failed" );
        else
        {
          tearDownOnMainThread();
        }
      }
      break;
    case ASYNC_RUN_TEST:
      {
        if ( m_result )
          m_result->protect( MethodFunctor( this, &TestCaller::runTestOnMainThread ),
                             this );
        else
        {
          runTestOnMainThread();
        }
      }
      break;
    default:
      break;
    }
    if ( m_event )
    {
      m_event->post();
    }
  }

private:
  TestCaller( const TestCaller &other );
  TestCaller &operator =( const TestCaller &other );

private:
  bool m_ownFixture;
  Fixture *m_fixture;
  TestMethod m_test;

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

#endif // CPPUNIT_TESTCALLER_H
