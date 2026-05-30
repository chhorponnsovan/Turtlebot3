#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from motor_msgs.msg import Encoderfeedback # Matches your workspace structure

import threading
import time
import matplotlib.pyplot as plt

class EncoderCalibrationNode(Node):
    def __init__(self):
        super().__init__('encoder_calibration_node')
        
        # Publishers and Subscribers
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.encoder_sub = self.create_subscription(
            Encoderfeedback, 
            '/encoder_feedback', # Adjust this topic name if your hardware publishes elsewhere
            self.encoder_callback, 
            10
        )
        
        # Trial Configuration
        self.max_speed = 0.35
        self.num_trials = 10
        self.speed_step = self.max_speed / self.num_trials # 0.04 m/s increments
        
        # Data storage for current active trial window
        self.is_recording = False
        self.current_left_ticks = 0
        self.current_right_ticks = 0
        
        # Aggregated historical lists for final plotting
        self.target_speeds = []
        self.left_ticks_history = []
        self.right_ticks_history = []
        
        self.get_logger().info("Encoder Calibration Node Initialized.")
        
        # Start the interactive experiment loop in a separate thread 
        # so it doesn't block the ROS2 executor spin.
        self.experiment_thread = threading.Thread(target=self.run_experiment)
        self.experiment_thread.start()

    def encoder_callback(self, msg: Encoderfeedback):
        if self.is_recording:
            # Cumulative sampling or delta tracking depends on your firmware.
            # Assuming incoming data is streaming continuous cumulative encoder values:
            self.current_left_ticks = msg.position_l
            self.current_right_ticks = msg.position_r

    def publish_speed(self, linear_x):
        twist = Twist()
        twist.linear.x = float(linear_x)
        twist.angular.z = 0.0
        self.cmd_vel_pub.publish(twist)

    def run_experiment(self):
        time.sleep(2.0) # Let initialization settle
        
        print("\n=== Wheel Encoder Dynamic Calibration Test ===")
        print("You will control when each speed increment begins.")
        
        for trial in range(1, self.num_trials + 1):
            target_speed = round(trial * self.speed_step, 3)
            
            print("\n--------------------------------------------------")
            print(f"Trial {trial}/{self.num_trials} Ready | Target Speed: {target_speed} m/s")
            input("Press [ENTER] to start this trial (runs for 10 seconds)...")
            
            # 1. Reset / Sample Initial Ticks
            start_left = self.current_left_ticks
            start_right = self.current_right_ticks
            self.is_recording = True
            
            # 2. Command the robot to drive forward
            self.publish_speed(target_speed)
            self.get_logger().info(f"Driving at {target_speed} m/s...")
            
            # 3. Maintain speed for 10-second trial window
            time.sleep(10.0)
            
            # 4. Record cumulative window deltas
            self.is_recording = False
            delta_left = abs(self.current_left_ticks - start_left)
            delta_right = abs(self.current_right_ticks - start_right)
            
            # Stop motor briefly between manual steps for structural safety
            self.publish_speed(0.0)
            
            # Append data to tracking history
            self.target_speeds.append(target_speed)
            self.left_ticks_history.append(delta_left)
            self.right_ticks_history.append(delta_right)
            
            print(f"\n[Trial {trial} Results Collected]")
            print(f"  Left Wheel Delta Ticks:  {delta_left}")
            print(f"  Right Wheel Delta Ticks: {delta_right}")
            
            # Option to repeat or proceed
            retry = input("Satisfied? Press [ENTER] to lock data or type 'r' to re-run this speed step: ")
            if retry.strip().lower() == 'r':
                # Remove last data entries and decrement counter to repeat loop step
                self.target_speeds.pop()
                self.left_ticks_history.pop()
                self.right_ticks_history.pop()
                trial -= 1
                continue

        print("\nAll trials completed! Stopping execution and building data plots...")
        self.stop_robot_and_plot()

    def stop_robot_and_plot(self):
        # Strict fallback to absolute zero
        self.publish_speed(0.0)
        
        # Plot generation block
        plt.figure(figsize=(10, 6))
        plt.plot(self.target_speeds, self.left_ticks_history, 'bo-', label='Left Wheel Ticks')
        plt.plot(self.target_speeds, self.right_ticks_history, 'ro-', label='Right Wheel Ticks')
        
        plt.title('Encoder Feedback Comparison across Target Speeds (10s Window)')
        plt.xlabel('Target Linear Velocity (m/s)')
        plt.ylabel('Cumulative Tick Delta')
        plt.grid(True, linestyle='--', alpha=0.6)
        plt.legend()
        
        print("\n--- Summary Performance Matrix ---")
        print(f"{'Target Speed':<15}{'Left Ticks':<15}{'Right Ticks':<15}{'Mismatch (%)':<15}")
        for i in range(len(self.target_speeds)):
            spd = self.target_speeds[i]
            l_tk = self.left_ticks_history[i]
            r_tk = self.right_ticks_history[i]
            mismatch = round((abs(l_tk - r_tk) / max(1, l_tk)) * 100, 2)
            print(f"{spd:<15}{l_tk:<15}{r_tk:<15}{mismatch}%")
            
        plt.show()

    def destroy_node(self):
        # Emergency hook redundancy for node destructions via Ctrl+C mid-run
        self.get_logger().info("Shutting down safely. Sending zero velocity to /cmd_vel.")
        self.publish_speed(0.0)
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = EncoderCalibrationNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()