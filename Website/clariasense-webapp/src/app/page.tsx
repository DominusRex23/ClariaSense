'use client';
import Image from 'next/image'
import {onValue, ref, off } from 'firebase/database';
import {database} from './library/firebaseconfig'
import { useEffect, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import Link from 'next/link'; // Import Next.js Link

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

  const [menuOpen, setMenuOpen] = useState(false);
  const [data, setData] = useState<{ id: string; value: number }[]>([]);

  useEffect(() => {
    const sensorsRef = ref(database, "sensors");

    const handleValueChange = (snapshot: any) => {
      const sensorsData = snapshot.val();
      if (sensorsData) {
      const values = Object.entries(sensorsData).map(([id, value]) => ({ id, value: Number(value) }));
      console.log("Fetched values:", values);
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
                      <Image
                        src="/clariaSenseLogo.png"
                        width={100}
                        height={67}
                        alt="Logo"
                      />
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
      
      const borderColor = value.id === 'ph' 
        ? 'border-red-400'
        : value.id === 'tds'
        ? 'border-yellow-400'
        : value.id === 'temp'
        ? 'border-blue-400'
        : 'border-gray-300';

      const valueColor = value.id === 'ph'
        ? 'text-red-500'
        : value.id === 'tds'
        ? 'text-yellow-500'
        : value.id === 'temp'
        ? 'text-blue-500'
        : 'text-gray-800';

      return (
        <div
          key={index}
          className={`bg-white rounded-2xl shadow-md p-6 text-center border-t-4 ${borderColor} hover:shadow-xl transition-all duration-300`}>
          <p className="text-xl font-semibold text-gray-600 mb-2">{sensor}</p>
          <p className={`text-6xl font-bold ${valueColor}`}>
            {value.value}
            <span className="text-2xl text-gray-500 ml-1">{unit}</span>
          </p>
        </div>
      );
    })}
  </div>
</main>

        </div>
    );
}
