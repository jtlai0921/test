//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile$
//
//  Version     : $Revision$
//
//  Description : implemets Unit Test Log
// ***************************************************************************

#ifndef BOOST_TEST_UNIT_TEST_LOG_IPP_012205GER
#define BOOST_TEST_UNIT_TEST_LOG_IPP_012205GER

// Boost.Test
#include <boost/test/unit_test_log.hpp>
#include <boost/test/unit_test_log_formatter.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/unit_test_parameters.hpp>

#include <boost/test/utils/basic_cstring/compare.hpp>
#include <boost/test/utils/foreach.hpp>

#include <boost/test/output/compiler_log_formatter.hpp>
#include <boost/test/output/xml_log_formatter.hpp>

// Boost
#include <boost/shared_ptr.hpp>
#include <boost/io/ios_state.hpp>
typedef ::boost::io::ios_base_all_saver io_saver_type;

#include <boost/test/detail/suppress_warnings.hpp>

//____________________________________________________________________________//

namespace boost {
namespace unit_test {

// ************************************************************************** //
// **************             entry_value_collector            ************** //
// ************************************************************************** //

namespace ut_detail {

entry_value_collector const&
entry_value_collector::operator<<( lazy_ostream const& v ) const
{
    unit_test_log << v;

    return *this;
}

//____________________________________________________________________________//

entry_value_collector const&
entry_value_collector::operator<<( const_string v ) const
{
    unit_test_log << v;

    return *this;
}

//____________________________________________________________________________//

entry_value_collector::~entry_value_collector()
{
    if( m_last )
        unit_test_log << log::end();
}

//____________________________________________________________________________//

} // namespace ut_detail

// ************************************************************************** //
// **************                 unit_test_log                ************** //
// ************************************************************************** //

namespace {

// log data
struct unit_test_log_data_helper_impl {
  typedef boost::shared_ptr<unit_test_log_formatter> formatter_ptr;
  typedef boost::shared_ptr<io_saver_type>           saver_ptr;

  bool                m_enabled;
  std::ostream*       m_stream;
  saver_ptr           m_stream_state_saver;
  formatter_ptr       m_log_formatter;

  unit_test_log_data_helper_impl(unit_test_log_formatter* p_log_formatter, bool enabled = false)
    : m_log_formatter()
    , m_stream( &std::cout )
    , m_stream_state_saver( new io_saver_type( std::cout ) )
    , m_enabled(enabled)
  {
    m_log_formatter.reset(p_log_formatter);
    m_log_formatter->set_log_level(log_all_errors);
  }

  // helper functions
  std::ostream&       stream()
  {
      return *m_stream;
  }

  log_level get_log_level() const
  {
      return m_log_formatter->get_log_level();
  }
};

struct unit_test_log_impl {
    // Constructor
    unit_test_log_impl()
    {
      m_log_formatter_data.push_back( unit_test_log_data_helper_impl(new output::compiler_log_formatter, true) );
      m_log_formatter_data.push_back( unit_test_log_data_helper_impl(new output::xml_log_formatter, false) );
    }

    typedef std::vector<unit_test_log_data_helper_impl> v_formatter_data_t;
    v_formatter_data_t m_log_formatter_data;

    // entry data
    log_entry_data      m_entry_data;
    bool                m_entry_in_progress;

    // check point data
    log_checkpoint_data m_checkpoint_data;

    void                set_checkpoint( const_string file, std::size_t line_num, const_string msg )
    {
        assign_op( m_checkpoint_data.m_message, msg, 0 );
        m_checkpoint_data.m_file_name   = file;
        m_checkpoint_data.m_line_num    = line_num;
    }
};

unit_test_log_impl& s_log_impl() { static unit_test_log_impl the_inst; return the_inst; }

} // local namespace

//____________________________________________________________________________//

void
unit_test_log_t::test_start( counter_t test_cases_amount )
{

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
      if( current_logger_data.get_log_level() == log_nothing )
          continue;

      current_logger_data.m_log_formatter->log_start( current_logger_data.stream(), test_cases_amount );

      if( runtime_config::get<bool>( runtime_config::BUILD_INFO ) )
          current_logger_data.m_log_formatter->log_build_info( current_logger_data.stream() );

      s_log_impl().m_entry_in_progress = false;
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_finish()
{
    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
      if( current_logger_data.get_log_level() == log_nothing )
          continue;

      current_logger_data.m_log_formatter->log_finish( current_logger_data.stream() );

      current_logger_data.stream().flush();
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_aborted()
{
    BOOST_TEST_LOG_ENTRY( log_messages ) << "Test is aborted";
}

//____________________________________________________________________________//

void
unit_test_log_t::test_unit_start( test_unit const& tu )
{
    if( s_log_impl().m_entry_in_progress )
        *this << log::end();
    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
        if( current_logger_data.get_log_level() > log_test_units )
            continue;
        current_logger_data.m_log_formatter->test_unit_start( current_logger_data.stream(), tu );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_unit_finish( test_unit const& tu, unsigned long elapsed )
{
    // Raffi: TODO check that things, used to be after the log level check
    s_log_impl().m_checkpoint_data.clear();

    if( s_log_impl().m_entry_in_progress )
        *this << log::end();

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {

        if( current_logger_data.get_log_level() > log_test_units )
            continue;

        current_logger_data.m_log_formatter->test_unit_finish( current_logger_data.stream(), tu, elapsed );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::test_unit_skipped( test_unit const& tu, const_string reason )
{
    // Raffi: this test used to be after checking for the log level
    if( s_log_impl().m_entry_in_progress )
        *this << log::end();

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
        if( current_logger_data.get_log_level() > log_test_units )
            continue;

        current_logger_data.m_log_formatter->test_unit_skipped( current_logger_data.stream(), tu, reason );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::exception_caught( execution_exception const& ex )
{
    log_level l =
        ex.code() <= execution_exception::cpp_exception_error   ? log_cpp_exception_errors :
        (ex.code() <= execution_exception::timeout_error        ? log_system_errors
                                                                : log_fatal_errors );

    // Raffi: initially after the check of the log level
    if( s_log_impl().m_entry_in_progress )
        *this << log::end();

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {

      if( l >= current_logger_data.get_log_level() ) {

          current_logger_data.m_log_formatter->log_exception_start( current_logger_data.stream(), s_log_impl().m_checkpoint_data, ex );

          log_entry_context( l );

          current_logger_data.m_log_formatter->log_exception_finish( current_logger_data.stream() );
      }
    }
    clear_entry_context();
}

//____________________________________________________________________________//

void
unit_test_log_t::set_checkpoint( const_string file, std::size_t line_num, const_string msg )
{
    s_log_impl().set_checkpoint( file, line_num, msg );
}

//____________________________________________________________________________//

char
set_unix_slash( char in )
{
    return in == '\\' ? '/' : in;
}

unit_test_log_t&
unit_test_log_t::operator<<( log::begin const& b )
{
    if( s_log_impl().m_entry_in_progress )
        *this << log::end();

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
        current_logger_data.m_stream_state_saver->restore();
    }

    s_log_impl().m_entry_data.clear();

    assign_op( s_log_impl().m_entry_data.m_file_name, b.m_file_name, 0 );

    // normalize file name
    std::transform( s_log_impl().m_entry_data.m_file_name.begin(), s_log_impl().m_entry_data.m_file_name.end(),
                    s_log_impl().m_entry_data.m_file_name.begin(),
                    &set_unix_slash );

    s_log_impl().m_entry_data.m_line_num = b.m_line_num;

    return *this;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( log::end const& )
{
    if( s_log_impl().m_entry_in_progress ) {
        log_entry_context( s_log_impl().m_entry_data.m_level );

       BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
            current_logger_data.m_log_formatter->log_entry_finish( current_logger_data.stream() );
       }

        s_log_impl().m_entry_in_progress = false;
    }

    clear_entry_context();

    return *this;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( log_level l )
{
    s_log_impl().m_entry_data.m_level = l;

    return *this;
}

//____________________________________________________________________________//

ut_detail::entry_value_collector
unit_test_log_t::operator()( log_level l )
{
    *this << l;

    return ut_detail::entry_value_collector();
}

//____________________________________________________________________________//

bool
unit_test_log_t::log_entry_start()
{
    if( s_log_impl().m_entry_in_progress )
        return true;

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {

        switch( s_log_impl().m_entry_data.m_level ) {
        case log_successful_tests:
            current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                                  unit_test_log_formatter::BOOST_UTL_ET_INFO );
            break;
        case log_messages:
            current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                                  unit_test_log_formatter::BOOST_UTL_ET_MESSAGE );
            break;
        case log_warnings:
            current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                                  unit_test_log_formatter::BOOST_UTL_ET_WARNING );
            break;
        case log_all_errors:
        case log_cpp_exception_errors:
        case log_system_errors:
            current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                                  unit_test_log_formatter::BOOST_UTL_ET_ERROR );
            break;
        case log_fatal_errors:
            current_logger_data.m_log_formatter->log_entry_start( current_logger_data.stream(), s_log_impl().m_entry_data,
                                                  unit_test_log_formatter::BOOST_UTL_ET_FATAL_ERROR );
            break;
        case log_nothing:
        case log_test_units:
        case invalid_log_level:
            return false;
        }

    }

    s_log_impl().m_entry_in_progress = true;

    return true;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( const_string value )
{
    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {

        if( s_log_impl().m_entry_data.m_level >= current_logger_data.get_log_level() && !value.empty() && log_entry_start() )
            current_logger_data.m_log_formatter->log_entry_value( current_logger_data.stream(), value );

    }
    return *this;
}

//____________________________________________________________________________//

unit_test_log_t&
unit_test_log_t::operator<<( lazy_ostream const& value )
{
    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
        if( s_log_impl().m_entry_data.m_level >= current_logger_data.get_log_level() && !value.empty() && log_entry_start() )
            current_logger_data.m_log_formatter->log_entry_value( current_logger_data.stream(), value );
    }
    return *this;
}

//____________________________________________________________________________//

void
unit_test_log_t::log_entry_context( log_level l )
{
    framework::context_generator const& context = framework::get_context();
    if( context.is_empty() )
        return;

    const_string frame;

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
        current_logger_data.m_log_formatter->entry_context_start( current_logger_data.stream(), l );
    }

    while( !(frame=context.next()).is_empty() )
    {
        BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
            current_logger_data.m_log_formatter->log_entry_context( current_logger_data.stream(), frame );
        }
    }

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
        current_logger_data.m_log_formatter->entry_context_finish( current_logger_data.stream() );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::clear_entry_context()
{
    framework::clear_context();
}

//____________________________________________________________________________//

void
unit_test_log_t::set_stream( std::ostream& str )
{
    if( s_log_impl().m_entry_in_progress )
        return;

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
       current_logger_data.m_stream = &str;
       current_logger_data.m_stream_state_saver.reset( new io_saver_type( str ) );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::set_threshold_level( log_level lev )
{
    if( s_log_impl().m_entry_in_progress || lev == invalid_log_level )
        return;

    BOOST_TEST_FOREACH( unit_test_log_data_helper_impl&, current_logger_data, s_log_impl().m_log_formatter_data ) {
        current_logger_data.m_log_formatter->set_log_level( lev );
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::set_format( output_format log_format )
{
    if( s_log_impl().m_entry_in_progress )
        return;

    switch( log_format ) {
    default:
    case OF_CLF:
        set_formatter( new output::compiler_log_formatter );
        break;
    case OF_XML:
        set_formatter( new output::xml_log_formatter );
        break;
    }
}

//____________________________________________________________________________//

void
unit_test_log_t::set_formatter( unit_test_log_formatter* the_formatter )
{
    // remove all previous loggers
    s_log_impl().m_log_formatter_data.clear();
    s_log_impl().m_log_formatter_data.push_back( unit_test_log_data_helper_impl(the_formatter, true) );
}

//____________________________________________________________________________//

// ************************************************************************** //
// **************            unit_test_log_formatter           ************** //
// ************************************************************************** //

void
unit_test_log_formatter::log_entry_value( std::ostream& ostr, lazy_ostream const& value )
{
    log_entry_value( ostr, (wrap_stringstream().ref() << value).str() );
}

//____________________________________________________________________________//

} // namespace unit_test
} // namespace boost

#include <boost/test/detail/enable_warnings.hpp>

#endif // BOOST_TEST_UNIT_TEST_LOG_IPP_012205GER
