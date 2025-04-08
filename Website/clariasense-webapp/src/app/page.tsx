'use client';
import Image from 'next/image'
import {onValue, ref, off } from 'firebase/database';
import { database, firestore } from './library/firebaseconfig';
import { collection, addDoc, serverTimestamp, query, getDocs, where } from "firebase/firestore";
import { useEffect, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import ReactPlayer from 'react-player';
import Link from 'next/link'; 

export default function Home() {
  const SENSOR_LABELS: Record<string, string> = {
    ph: "pH Level",
    tds: "TDS",
    temp: "Temperature",
  };  
  const SENSOR_UNITS: Record<string, string> = {
    ph: "pH",
    tds: "ppm",
    temp: "°C",
  };  
  const NORMAL_RANGES: Record<string, { min: number; max: number; text: string }> = {
    ph: { min: 6.5, max: 7.5, text: "Normal Range: 6.5pH - 7.5pH" },
    tds: { min: 100, max: 125, text: "Normal Range: 100ppm - 125ppm" },
    temp: { min: 29, max: 32, text: "Normal Range: 29°C - 32°C" },
  };

  const [email, setEmail] = useState("");
  const [menuOpen, setMenuOpen] = useState(false);
  const [data, setData] = useState<{ id: string; value: number }[]>([]);

  // Function to check if value is out of range
  const isOutOfRange = (id: string, value: number) => {
    const range = NORMAL_RANGES[id];
    if (!range) return false;
    return value < range.min || value > range.max;
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
  
    const userRef = collection(firestore, "subscribers");
  
    // Normalize email (trim + lowercase)
    const normalizedEmail = email.trim().toLowerCase();
  
    try {
      // Check if email already exists
      const existingQuery = query(userRef, where("email", "==", normalizedEmail));
      const querySnapshot = await getDocs(existingQuery);
  
      if (!querySnapshot.empty) {
        alert("You're already subscribed with this email.");
        return;
      }
  
      // Add new subscription
      await addDoc(userRef, {
        email: normalizedEmail,
        subscribedAt: serverTimestamp(),
      });
  
      alert("Subscription successful! You will receive updates soon.");
      setEmail(""); // Clear input field
  
    } catch (error) {
      console.error("Error subscribing:", error);
      alert("Failed to subscribe.");
    }
  };
  
  useEffect(() => {
    const sensorsRef = ref(database, "sensors");

    const handleValueChange = (snapshot: any) => {
      const sensorsData = snapshot.val();
      if (sensorsData) {
      const values = Object.entries(sensorsData).map(([id, value]) => ({ id, value: Number(value) }));
      setData(values);
      }
    };

    onValue(sensorsRef, handleValueChange, (error) => {
      console.error("Error fetching data: ", error);
    });

    return () => {
      off(sensorsRef, "value", handleValueChange);
    };
  }, []);

    const toggleMenu = () => {
        setMenuOpen(!menuOpen);
    };


    return (
        <div className='font-inter'>
            <motion.nav 
                initial={{ height: "auto" }}
                animate={{ height: menuOpen ? "auto" : "auto" }}
                transition={{ duration: 0.5, ease: "easeInOut" }}
                className="bg-white fixed w-full z-20 top-0 start-0 border-b border-gray-200"
            >
                <div className="max-w-screen-xl flex flex-wrap items-center justify-between mx-auto p-4">
                    {/* Logo Here */}
                    <div className=''>
                      <Link href="/">
                      <Image
                        src="/clariaSenseLogo.png"
                        width={100}
                        height={67}
                        alt="Logo"
                      />
                      </Link>
                    </div>

                    {/* Mobile menu button */}
                    <div className='lg:hidden'>
                      <button onClick={toggleMenu} className="text-4xl" title="Toggle Menu">
                        <svg xmlns="http://www.w3.org/2000/svg" 
                            className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d={menuOpen ? "M6 18L18 6M6 6l12 12" : "M4 6h16M4 12h16M4 18h16"} />
                        </svg>
                      </button>
                    </div>

                    
                    {/* Desktop Menu */}
<div className="hidden lg:flex space-x-6 items-center text-lg font-medium">
  <Link href="/"className="text-gray-700 hover:bg-blue-100 hover:text-blue-600 px-4 py-2 rounded-md transition duration-300">Home</Link>
  <Link href="/logs"className="text-gray-700 hover:bg-blue-100 hover:text-blue-600 px-4 py-2 rounded-md transition duration-300">Logs</Link>
  <Link href="/about"className="text-gray-700 hover:bg-blue-100 hover:text-blue-600 px-4 py-2 rounded-md transition duration-300">About</Link>
</div>

                </div>

                {/* Mobile Menu */}
                <AnimatePresence>
                    {menuOpen && (
                      <motion.div
                        initial={{ opacity: 0, y: -20 }}
                        animate={{ opacity: 1, y: 0 }}
                        exit={{ opacity: 0, y: -20 }}
                        transition={{ duration: 0.3 }}
                        className="lg:hidden"
                      >
                        <ul className='space-y-4 mx-5 py-2'>
                          <li><Link href="/" className='text-black opacity-70 hover:opacity-100 duration-300'>Home</Link></li>
                          <li><Link href="/logs" className='text-black opacity-70 hover:opacity-100 duration-300'>Logs</Link></li>
                          <li><Link href="/about" className='text-black opacity-70 hover:opacity-100 duration-300'>About</Link></li>
                        </ul>
                      </motion.div>
                    )}
                </AnimatePresence>
            </motion.nav>

            <main className="mx-8 mt-40">
  <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6 justify-center items-center">
    {data.map((value, index) => {
      const sensor = SENSOR_LABELS[value.id] ?? value.id;
      const unit = SENSOR_UNITS[value.id] ?? " ";
      const normalRange = NORMAL_RANGES[value.id]?.text ?? "";
      const isOutOfNormalRange = isOutOfRange(value.id, value.value);
      
      const borderColor = value.id === 'ph' 
        ? 'border-blue-400'
        : value.id === 'tds'
        ? 'border-blue-400'
        : value.id === 'temp'
        ? 'border-blue-400'
        : 'border-gray-300';

      const valueColor = value.id === 'ph'
        ? 'text-blue-500'
        : value.id === 'tds'
        ? 'text-blue-500'
        : value.id === 'temp'
        ? 'text-blue-500'
        : 'text-gray-800';

        const boxClasses = isOutOfNormalRange 
            ? 'bg-red-50 border-red-500 text-red-800' 
            : `bg-white ${
                value.id === 'ph' ? 'border-blue-400' :
                value.id === 'tds' ? 'border-blue-400' :
                value.id === 'temp' ? 'border-blue-400' :
                'border-gray-300'
              }`;

      return (
        <div
        key={index}
        className={`rounded-2xl shadow-md p-6 text-center border-t-4 hover:shadow-xl transition-all duration-300 overflow-hidden ${boxClasses}`}>
        <p className="text-xl font-semibold mb-2">{sensor}</p>
        <p className={`text-6xl font-bold font-medium break-words text-[clamp(2rem,5vw,3.5rem)] leading-tight ${
          isOutOfNormalRange ? 'text-red-600' : 
          value.id === 'ph' ? 'text-blue-500' :
          value.id === 'tds' ? 'text-blue-500' :
          value.id === 'temp' ? 'text-blue-500' :
          'text-gray-800'
        }`}>
          {value.value}
          <span className={`text-2xl ml-1 ${
            isOutOfNormalRange ? 'text-red-600' : 'text-gray-500'
          }`}>
            {unit}
          </span>
        </p>
        <p className={`text-sm mt-2 ${
          isOutOfNormalRange ? 'text-red-600' : 'text-gray-500'
        }`}>
          {normalRange}
        </p>
      </div>
    );
  })}
</div>
  <br />
            <br />

            
<div className="mt-12 mx-auto max-w-4xl px-4">
  <h2 className="text-2xl font-bold text-center mb-6">Our Product in Action</h2>
  <div className=" aspect-video  overflow-hidden shadow-xl">
  <ReactPlayer
      url="https://www.youtube.com/watch?v=XbLemOwzdxk" // Replace with your video URL
      width="100%"
      height="100%"
      controls={true}
      light={false}
      playing={false}
    />
  </div>
  <p className="text-center text-gray-600 mt-4">
    See how our monitoring system works in real aquarium environments
  </p>
</div> 
<br/>
<br/>
            <p className="text-center text-gray-500 mt-4">Want to know if the Aquarium is not in the best environment for fish?</p>
            <p className="text-center text-gray-500">Subscribe to our newsletter for updates!</p>
            <form 
            onSubmit={handleSubmit} 
            className="flex flex-col sm:flex-row items-center space-y-4 sm:space-y-0 sm:space-x-4 mt-8 mx-auto max-w-md p-4 w-full mb-20"
            >
            <input
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            placeholder="Enter your email"
            required
            className="flex-1 px-4 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 w-full sm:w-auto"
            />
            <button 
            type="submit" 
            className="bg-[#1341b1] text-white py-2 px-4 rounded-lg hover:bg-blue-600 transition duration-300 w-full sm:w-auto"
            >
            Subscribe
            </button>
            </form>
             </main>
             <footer className="bg-white border-t border-gray-200 py-4 mt-8 w-full fixed bottom-0 left-0">
          <div className="max-w-screen-xl mx-auto px-4 flex flex-col sm:flex-row justify-between items-center">
            <p className="text-gray-500 text-sm text-center w-full">
          © {new Date().getFullYear()} ClariaSense. CPE 4 - 3 Group 7 S.Y. 2024-2025. All rights reserved.
            </p>
          </div>
          </footer>
        </div>
    );
}