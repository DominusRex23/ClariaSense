'use client';

import { useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';

export default function Home() {
    const [menuOpen, setMenuOpen] = useState(false);

    const toggleMenu = () => {
        setMenuOpen(!menuOpen);
    };

    return (
        <div>
            <motion.nav 
            initial={{ height: "auto" }}
            animate={{ height: menuOpen ? "auto" : "auto" }}
            transition={{ duration: 0.5, ease: "easeInOut" }}
            className="bg-indigo-600 text-white shadow-md overflow-hidden">
                <div className="container px-4 py-3 h-full mx-auto flex justify-between items-center">
                    {/* Logo Here */}
                    <div className='text-2xl font-bold'>
                      Logo
                    </div>

                    {/* Menu links here */}
                    <div className='lg:hidden'>
                      <button onClick={toggleMenu} 
                      className="text-white text-4xl" title="Toggle Menu">
                      <svg xmlns="http://www.w3.org/2000/svg" 
                          className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d={menuOpen ? "M6 18L18 6M6 6l12 12" : "M4 6h16M4 12h16M4 18h16"} />
                      </svg>
                      </button>
                  </div>
                    <div className='hidden lg:flex space-x-6'>
                      <a className='text-white opacity-70 hover:opacity-100 duration-300' href='#'>Home</a>
                      <a className='text-white opacity-70 hover:opacity-100 duration-300' href='#'>Logs</a>
                      <a className='text-white opacity-70 hover:opacity-100 duration-300' href='#'>About</a>
                    </div>
                  </div>
                    <AnimatePresence>
                    {/* Mobile menu button */}
                    {menuOpen && (
                      <motion.div
                      initial={{ opacity: 0, y: -20 }}
                      animate={{ opacity: 1, y: 0 }}
                      exit={{ opacity: 0, y: -20 }}
                      transition={{ duration: 0.3 }}
                      className="lg:hidden"
                    >
                        <ul className='space-y-4 mx-5 py-2'>
                          <li><a className='text-white opacity-70 hover:opacity-100 duration-300' href='#'>Home</a></li>
                          <li><a className='text-white opacity-70 hover:opacity-100 duration-300' href='#'>Logs</a></li>
                          <li><a className='text-white opacity-70 hover:opacity-100 duration-300' href='#'>About</a></li>
                        </ul>
                      </motion.div>
                    )}
                    </AnimatePresence>
            </motion.nav>
            <main className="text-center mt-40">
                <h1 className="text-4xl">TEST</h1>
                <h2 className="mt-10 text-xl">Responsive Navigation Bar Example</h2>
            </main>
        </div>
    );
}
