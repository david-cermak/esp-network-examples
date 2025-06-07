#!/usr/bin/env python3
# Designed for use with boofuzz v0.2.0
#
# A partial MDNS fuzzer. Could be made to be a DNS fuzzer trivially
# Charlie Miller <cmiller@securityevaluators.com>
# Updated to exercise IoT devices with monitoring capabilities

import getopt  # command line arguments
import platform  # For getting the operating system name
import subprocess  # For executing a shell command
import sys
import time
from boofuzz import *

mylogger = FuzzLoggerText()
g_target_ip_addr = None
g_mdns_port = 5353


# Verify if the IOT target is still alive by expecting a response to a ping. This means that the test must have
# network access to the same subnet as the IOT device target. Verify that a ping reply is successful independent of
# running this fuzz test.
#
# noinspection PyMethodOverriding
# noinspection PyMethodParameters
class IOT_TargetMonitor(NetworkMonitor):
    def alive():
        global g_target_ip_addr
        param = "-n" if platform.system().lower() == "windows" else "-c"
        # One ping only
        command = ["ping", param, "1", g_target_ip_addr]
        # noinspection PyTypeChecker
        message = "alive() sending a ping command to " + g_target_ip_addr
        mylogger.log_info(message)
        try:
            subprocess.run(command, timeout=3)
        except subprocess.TimeoutExpired:
            return False
        else:
            mylogger.log_info("PING success")
            return True

    def pre_send(target=None, fuzz_data_logger=None, session=None, **kwargs):
        return

    def post_send(target=None, fuzz_data_logger=None, session=None, **kwargs):
        return True

    def retrieve_data():
        return

    def start_target():
        return True

    def set_options(*args, **kwargs):
        return

    def get_crash_synopsis():
        return "get_crash_synopsis detected a crash of the target."

    # The use of a 12 second sleep is based on experimentation for a specific IOT device. Change the number of seconds
    # as needed for your environment.
    def restart_target(target=None, **kwargs):
        mylogger.log_info("restart_target sleep for 12")
        time.sleep(12)
        if IOT_TargetMonitor.alive() is True:
            mylogger.log_info("restart_target ok")
            return True
        else:
            mylogger.log_info("restart_target failed")
            return False

    def post_start_target(target=None, fuzz_data_logger=None, session=None, **kwargs):
        return


def insert_questions(target, fuzz_data_logger, session, node, edge, *args, **kwargs):
    # print(node.names)
    node.names["query.Questions"].value = 1 + node.names["query.queries"].current_reps
    node.names["query.Authority"].value = 1 + node.names["query.auth_nameservers"].current_reps


def main(argv):
    # parse command line options.
    target_ip_addr = None
    mdns_port = 5353
    start_index = 1
    end_index = 1000

    global g_target_ip_addr
    global g_mdns_port

    try:
        opts, args = getopt.getopt(argv, "ha:p:s:e:", ["address=", "port=", "start_index=", "end_index="])
    except getopt.GetoptError:
        print(
            "mdns.py --address|-a <target ip address> --port|-p <MDNS port> --start_index|\
            -s <start of fuzzing index> --end_index|-e <end of fuzzing index>"
        )
        sys.exit(2)
    for opt, arg in opts:
        if opt == "-h":
            print(
                "mdns.py --address|-a <target ip address> --port|-p <MDNS port> --start_index|\
                -s <start of fuzzing index> --end_index|-e <end of fuzzing index>"
            )
            sys.exit()
        elif opt in ("-a", "--address"):
            target_ip_addr = arg
        elif opt in ("-p", "--port"):
            mdns_port = arg
        elif opt in ("-s", "--startindex"):
            start_index = arg
        elif opt in ("-e", "--endindex"):
            end_index = arg

    if target_ip_addr is None:
        print("Error: Target IP address is required. Use -a or --address option.")
        sys.exit(2)

    g_target_ip_addr = target_ip_addr
    g_mdns_port = int(mdns_port)
    target_message = "Target device ip address and port " + str(g_target_ip_addr) + " " + str(g_mdns_port)
    mylogger.log_info(target_message)

    IOT_TargetMonitor(host=g_target_ip_addr, port=g_mdns_port)

    # Define the MDNS protocol
    s_initialize("query")
    s_word(0, name="TransactionID")
    s_word(0, name="Flags")
    s_word(1, name="Questions", endian=">")
    s_word(0, name="Answer", endian=">")
    s_word(1, name="Authority", endian=">")
    s_word(0, name="Additional", endian=">")

    # ######## Queries ################
    if s_block_start("query"):
        if s_block_start("name_chunk"):
            s_size("string", length=1)
            if s_block_start("string"):
                s_string("A" * 10)
            s_block_end()
        s_block_end()
        s_repeat("name_chunk", min_reps=2, max_reps=4, step=1, fuzzable=True, name="aName")

        s_group("end", values=[b"\x00", b"\xc0\xb0"])  # very limited pointer fuzzing
        s_word(0xC, name="Type", endian=">")
        s_word(0x8001, name="Class", endian=">")
    s_block_end()
    s_repeat("query", 0, 1000, 40, name="queries")

    # ####### Authorities #############
    if s_block_start("auth_nameserver"):
        if s_block_start("name_chunk_auth"):
            s_size("string_auth", length=1)
            if s_block_start("string_auth"):
                s_string("A" * 10)
            s_block_end()
        s_block_end()
        s_repeat("name_chunk_auth", min_reps=2, max_reps=4, step=1, fuzzable=True, name="aName_auth")
        s_group("end_auth", values=[b"\x00", b"\xc0\xb0"])  # very limited pointer fuzzing

        s_word(0xC, name="Type_auth", endian=">")
        s_word(0x8001, name="Class_auth", endian=">")
        s_dword(0x78, name="TTL_auth", endian=">")
        s_size("data_length", length=2, endian=">")
        if s_block_start("data_length"):
            s_binary("00 00 00 00 00 16 c0 b0")  # This should be fuzzed according to the type, but I'm too lazy atm
        s_block_end()
    s_block_end()
    s_repeat("auth_nameserver", 0, 1000, 40, name="auth_nameservers")

    s_word(0)

    mylogger.log_info("Initializing session to target ")
    sess = Session(
        target=Target(
            connection=UDPSocketConnection(g_target_ip_addr, g_mdns_port),
            monitors=[IOT_TargetMonitor],
        ),
        # The use of a 12 second sleep is based on experimentation for a specific IOT device. Change the sleep count
        # as needed for your environment.
        sleep_time=12,
        crash_threshold_request=2,
        crash_threshold_element=2,
        index_start=int(start_index),
        index_end=int(end_index),
    )
    sess.connect(s_get("query"), callback=insert_questions)
    mylogger.log_info("start fuzzing")

    sess.fuzz()


if __name__ == "__main__":
    main(sys.argv[1:])
