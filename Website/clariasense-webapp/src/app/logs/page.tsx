'use client';
import Image from 'next/image';
import Link from 'next/link';
import { useEffect, useState } from 'react';
import { AnimatePresence, motion } from 'framer-motion';
import { FaFlask, FaTint, FaTemperatureHigh, FaExclamationTriangle } from 'react-icons/fa';
import { firestore } from '../library/firebaseconfig';
import { collection, getDocs, query, orderBy } from 'firebase/firestore';

// Fetch hourly logs
async function fetchLogsData() {
  const logsQuery = query(collection(firestore, 'hourly_logs'), orderBy('timestamp', 'desc'));
  const querySnapshot = await getDocs(logsQuery);
  return querySnapshot.docs.map(doc => ({ id: doc.id, ...doc.data() }));
}

// Fetch error logs
async function fetchErrorLogsData() {
  const logsQuery = query(collection(firestore, 'error_logs'), orderBy('timestamp', 'desc'));
  const querySnapshot = await getDocs(logsQuery);
  return querySnapshot.docs.map(doc => ({ id: doc.id, ...doc.data() }));
}

export default function Logs() {
  const [menuOpen, setMenuOpen] = useState(false);
  const [hourlyLogData, setHourlyLogData] = useState([]);
  const [errorLogData, setErrorLogData] = useState([]);
  const [filter, setFilter] = useState('logs');

  useEffect(() => {
    fetchLogsData().then(setHourlyLogData);
    fetchErrorLogsData().then(setErrorLogData);
  }, []);

  const toggleMenu = () => setMenuOpen(!menuOpen);

  return (
    <div>
      {/* NAVBAR */}
      <motion.nav
        initial={{ height: 'auto' }}
        animate={{ height: menuOpen ? 'auto' : 'auto' }}
        transition={{ duration: 0.5, ease: 'easeInOut' }}
        className="bg-white fixed w-full z-20 top-0 start-0 border-b border-gray-200"
      >
        <div className="max-w-screen-xl flex justify-between items-center mx-auto p-4">
          <Image src="/clariaSenseLogo.png" width={100} height={67} alt="Logo" />
          <div className="lg:hidden">
            <button onClick={toggleMenu} className="text-4xl" title="Toggle Menu">
              <svg className="h-6 w-6" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d={menuOpen ? 'M6 18L18 6M6 6l12 12' : 'M4 6h16M4 12h16M4 18h16'}
                />
              </svg>
            </button>
          </div>

          <div className="hidden lg:flex space-x-6 items-center text-lg font-medium">
            <Link href="/" className="text-gray-700 hover:bg-blue-100 hover:text-blue-600 px-4 py-2 rounded-md transition duration-300">
              Home
            </Link>
            <Link href="/logs" className="text-gray-700 hover:bg-blue-100 hover:text-blue-600 px-4 py-2 rounded-md transition duration-300">
              Logs
            </Link>
            <Link href="/about" className="text-gray-700 hover:bg-blue-100 hover:text-blue-600 px-4 py-2 rounded-md transition duration-300">
              About
            </Link>
          </div>
        </div>

        <AnimatePresence>
          {menuOpen && (
            <motion.div
              initial={{ opacity: 0, y: -20 }}
              animate={{ opacity: 1, y: 0 }}
              exit={{ opacity: 0, y: -20 }}
              transition={{ duration: 0.3 }}
              className="lg:hidden"
            >
              <ul className="space-y-4 mx-5 py-2">
                <li><Link href="/" className="mobile-link">Home</Link></li>
                <li><Link href="/logs" className="mobile-link">Logs</Link></li>
                <li><Link href="/about" className="mobile-link">About</Link></li>
              </ul>
            </motion.div>
          )}
        </AnimatePresence>
      </motion.nav>

      {/* MAIN CONTENT */}
      <main className="pt-28 px-6 sm:px-10">
        <div className="mb-6">
          <button
            onClick={() => setFilter('logs')}
            className={`filter-btn ${filter === 'logs' ? 'bg-[#1341b1] text-white' : 'hover:bg-blue-500 hover:text-white'}`}>
            Logs
          </button>
          <button
          onClick={() => setFilter('errorLogs')}
          className={`filter-btn error ${filter === 'errorLogs' ? 'bg-red-500 text-white' : ''}`}>
          Out of Parameter Logs
</button>
        </div>
      </main>

      {/* Display the logs */}
      {filter === 'logs' && (
        <div>
          <p className="italic mb-4 text-gray-500">This part is for experimental purposes, might remove depending on the requirements</p>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
            {hourlyLogData.map((logs, index) => (
              <motion.div
                key={logs.id}
                initial={{ opacity: 0, y: 10 }}
                animate={{ opacity: 1, y: 0 }}
                transition={{ duration: 0.3, delay: index * 0.1 }}
                className="log-card border-gray-300 bg-gray-100 hover:shadow-lg hover:shadow-blue-500/50"
              >
                <p className="text-sm font-medium text-gray-600 mb-2">
                  Time Recorded: <span className="text-gray-800 font-medium">{logs.timestamp}</span>
                </p>
                <div className="space-y-2 text-sm text-gray-700">
                  <p><FaFlask className="inline mr-2 text-blue-500" />pH: {Math.min(...logs.ph)} - {Math.max(...logs.ph)} pH</p>
                  <p><FaTint className="inline mr-2 text-purple-500" />TDS: {Math.min(...logs.tds)} - {Math.max(...logs.tds)} ppm</p>
                  <p><FaTemperatureHigh className="inline mr-2 text-orange-500" />Temp: {Math.min(...logs.temp)} - {Math.max(...logs.temp)} °C</p>
                </div>
              </motion.div>
            ))}
          </div>
        </div>
      )}

      {filter === 'errorLogs' && (
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
          {errorLogData.map((logs, index) => (
            <motion.div
              key={logs.id}
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ duration: 0.3, delay: index * 0.05 }}
              className="log-card border-gray-300 bg-gray-50 hover:shadow-lg hover:shadow-red-500/50"
            >
              <p className="text-sm font-medium text-gray-600 mb-2">
                Time Recorded: <span className="text-gray-800">{logs.timestamp}</span>
              </p>
              <p className="text-red-600 font-semibold text-sm mb-2">
                <FaExclamationTriangle className="inline mr-2" />
                Out of Range: {Array.isArray(logs.errorParameters) ? logs.errorParameters.join(', ') : 'N/A'}
              </p>
              <div className="space-y-2 text-sm text-gray-700">
                <p><FaFlask className="inline mr-2 text-blue-500" />pH: {logs.ph} pH</p>
                <p><FaTint className="inline mr-2 text-purple-500" />TDS: {logs.tds} ppm</p>
                <p><FaTemperatureHigh className="inline mr-2 text-orange-500" />Temp: {logs.temp} °C</p>
              </div>
            </motion.div>
          ))}
        </div>
      )}

      <style jsx>{`
        .nav-link {
          color: #374151;
          padding: 0.5rem 1rem;
          border-radius: 0.375rem;
          transition: all 0.3s ease;
        }
        .nav-link:hover {
          background-color: #ebf8ff;
          color: #1e40af;
        }
        .mobile-link {
          color: black;
          opacity: 0.7;
          transition: 0.3s;
        }
        .mobile-link:hover {
          opacity: 1;
        }
        .filter-btn {
          padding: 0.5rem 1rem;
          margin-right: 1rem;
          border-radius: 0.375rem;
          background-color: #e5e7eb;
          color: black;
          transition: all 0.3s;
          cursor: pointer;
        }
        .filter-btn:hover {
          background-color: #3b82f6; /* blue-500 */
          color: white;
        }
         .filter-btn.error:hover {
    background-color: #ef4444; /* red-500 */
        }  
        .log-card {
          padding: 1rem;
          border-radius: 0.75rem;
          border: 1px solid #d1d5db;
          background-color: white;
          box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
          transition: 0.3s ease;
        }
        .log-card:hover {
          box-shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
        }
      `}</style>
    </div>
  );
}
